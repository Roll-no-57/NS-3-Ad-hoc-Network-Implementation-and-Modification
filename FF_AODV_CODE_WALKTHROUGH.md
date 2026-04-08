# FF-AODV Code-Level Walkthrough (Phase 1 + Phase 2)

This document is a teacher-facing explanation of what was changed in ns-3 AODV, where it was changed, and why.

It is written so you can:
- explain the design clearly,
- map each idea to exact files/functions,
- quickly rewrite critical parts if code is removed during viva/presentation.

## 1) Big Picture

Standard AODV mostly prefers routes using sequence number freshness and lower hop count.

Your implementation introduces FF-AODV route quality using a fitness score.

- Phase 1 (paper implementation):
  $$F = \alpha \cdot \frac{E_{residual}}{E_{initial}} + \beta \cdot \frac{1}{HopCount}$$
- Phase 2 (modification):
  $$F = \alpha \cdot \frac{E_{residual}}{E_{initial}} + \beta \cdot \frac{1}{HopCount} + \gamma \cdot \frac{V_{max} - V}{V_{max}}$$

Current codebase supports both phases using attributes:
- Phase 1 behavior: set `Gamma = 0.0`
- Phase 2 behavior: set `Gamma > 0` (you use `0.2`)

## 2) Exactly Which Files Were Changed

Core protocol files under `ns-allinone-3.39/ns-3.39/src/aodv/`:

1. `CMakeLists.txt`
2. `model/aodv-packet.h`
3. `model/aodv-packet.cc`
4. `model/aodv-rtable.h`
5. `model/aodv-rtable.cc`
6. `model/aodv-routing-protocol.h`
7. `model/aodv-routing-protocol.cc`

Validation/demo files under `ns-allinone-3.39/ns-3.39/scratch/`:

1. `test-ff-aodv.cc` (Phase 1 style config)
2. `test-ff-aodv-v2.cc` (Phase 2 style config)
3. `experiment-ff-aodv.cc` (parameterized runner for CSV generation)

## 3) Change-by-Change Technical Explanation

### 3.1 `src/aodv/CMakeLists.txt`

### What changed
- Added `${libenergy}` to `LIBRARIES_TO_LINK` for AODV.

### Why
- Routing protocol now reads node residual energy from ns-3 energy models.
- Without linking energy library, `EnergySource`/`EnergySourceContainer` usage in routing code would fail.

---

### 3.2 `src/aodv/model/aodv-packet.h`

### What changed
Both control headers were extended:

1. `RreqHeader`:
   - Added `double m_pathFitness`
   - Added `double m_velocity`
   - Added methods:
     - `SetPathFitness(double)` / `GetPathFitness()`
     - `SetVelocity(double)` / `GetVelocity()`

2. `RrepHeader`:
   - Added `double m_pathFitness`
   - Added `double m_velocity`
   - Added same getter/setter pairs

### Why
- Fitness and velocity need to travel hop-by-hop inside RREQ/RREP packets.
- Route decision happens at intermediate nodes, so the metric must be in the control packet, not only local variables.

---

### 3.3 `src/aodv/model/aodv-packet.cc`

### What changed
1. Constructors initialize new fields to `0.0`.
2. `GetSerializedSize()` updated:
   - RREQ: `39` bytes (`23 + 8 + 8`)
   - RREP: `35` bytes (`19 + 8 + 8`)
3. `Serialize()` and `Deserialize()` now read/write doubles as network-order `uint64_t` using `memcpy`.
4. `operator==` checks include `m_pathFitness` and `m_velocity`.
5. `RrepHeader::Print()` now includes fitness and velocity.

### Why
- If header serialization is not updated, packet fields exist only in memory and are lost on wire.
- `uint64_t + memcpy` preserves exact IEEE-754 bits for doubles when serializing.

---

### 3.4 `src/aodv/model/aodv-rtable.h` and `aodv-rtable.cc`

### What changed
1. `RoutingTableEntry` gained `double m_fitness`.
2. Added:
   - `SetFitness(double)`
   - `GetFitness() const`
3. Constructor initializes `m_fitness(0.0)`.

### Why
- AODV route table must remember best fitness for each destination.
- Later duplicate RREQ/RREP updates compare against this stored value.

---

### 3.5 `src/aodv/model/aodv-routing-protocol.h`

### What changed
1. Added velocity dependency:
   - `#include "ns3/mobility-model.h"`
2. Added FF-AODV state:
   - `m_alpha`, `m_beta`, `m_initialEnergy`
   - `m_gamma`, `m_maxVelocity`
3. Fitness function signature:
   - `double CalculateFitness(double residualEnergy, uint8_t hopCount, double velocity) const;`

### Why
- This centralizes all optimization weights and allows them to be tuned via ns-3 attributes.

---

### 3.6 `src/aodv/model/aodv-routing-protocol.cc`

This is the most important file. The major algorithm is here.

#### A) Constructor defaults and attributes

### What changed
- Constructor defaults:
  - `m_alpha(0.5)`
  - `m_beta(0.3)`
  - `m_initialEnergy(100.0)`
  - `m_gamma(0.2)`
  - `m_maxVelocity(50.0)`
- `GetTypeId()` registers attributes:
  - `Alpha`, `Beta`, `InitialEnergy`, `Gamma`, `MaxVelocity`

### Why
- Enables scenario-level configuration from helper scripts and scratch simulations.

#### B) `CalculateFitness(...)`

### What changed
Computes:

1. `energyRatio = residualEnergy / m_initialEnergy` (guarded)
2. `hopFactor = 1.0 / hopCount` (guarded)
3. `velocityFactor = (m_maxVelocity - clampedVelocity) / m_maxVelocity` (guarded)
4. Returns weighted sum.

### Why
- Captures three competing route quality aspects:
  - longevity (energy),
  - efficiency (hop count),
  - stability (lower speed).

#### C) `RecvRequest(...)` (RREQ processing)

### What changed
After hop increment, FF-AODV logic does:

1. Reads local residual energy from `EnergySourceContainer`.
2. Reads local speed from `MobilityModel::GetVelocity()` and computes magnitude.
3. Computes local fitness with current hop count.
4. Path fitness update rule:
   - `pathFitness = min(incomingFitness, localFitness)`
   - If incoming fitness is zero, uses local fitness.
5. Path velocity update rule:
   - `pathVelocity = max(incomingVelocity, currentSpeed)`
6. Stores both in RREQ header.
7. Duplicate RREQ policy changed:
   - old AODV: duplicate usually dropped by ID cache.
   - here: duplicate accepted only if it improves reverse-route fitness.
8. Reverse route entry to origin stores/updates fitness via `SetFitness(pathFitness)`.
9. Existing reverse route is updated only when `pathFitness > existingFitness`.

### Why
- This is the core replacement of first-arrival / shortest-path bias.
- It allows better-quality (possibly later-arriving) routes to be considered.

#### D) `SendReply(...)` and `SendReplyByIntermediateNode(...)`

### What changed
1. `SendReply` seeds RREP fitness/velocity from incoming RREQ fields.
2. If fields are missing (compatibility fallback), recomputes local values.
3. Intermediate-node RREP also sets path fitness/velocity.
4. Gratuitous RREP path also gets fitness/velocity.

### Why
- Keeps metric continuity from request path to reply path.
- Avoids losing path metric at destination/intermediate response stage.

#### E) `RecvReply(...)` (RREP processing)

### What changed
1. Similar local energy/speed extraction.
2. Local reply fitness computed.
3. Combined reply fitness uses weakest-link rule (`min`).
4. Path velocity updated as max-so-far.
5. New/updated forward route entry stores `replyFitness`.
6. Route update decision for same destination sequence number changed:
   - old AODV uses lower hop count.
   - FF-AODV compares `replyFitness > storedFitness`.

### Why
- Ensures final selected forward route follows fitness policy end-to-end, not hop-count fallback.

## 4) Phase 1 vs Phase 2 in Current Unified Code

Your current implementation is unified, not split into separate branches.

- Phase 1 mode:
  - `Alpha=0.6`, `Beta=0.4`, `Gamma=0.0`
  - Velocity field exists in packets but has zero influence on score.
- Phase 2 mode:
  - `Alpha=0.5`, `Beta=0.3`, `Gamma=0.2`, `MaxVelocity=50.0`
  - Velocity actively affects route ranking.

This switch is visible in `scratch/experiment-ff-aodv.cc` (`protocolVersion` selects weight set).

## 5) Minimal End-to-End Flow (What Happens in Runtime)

1. Source starts route discovery (RREQ).
2. Each receiver of RREQ:
   - computes local fitness,
   - updates packet fitness (min),
   - updates packet velocity (max),
   - possibly keeps duplicate RREQ if fitness improves.
3. Destination/intermediate sends RREP carrying fitness/velocity.
4. Each receiver of RREP repeats aggregation and updates forward routes by fitness.
5. Data packets use routes selected by highest available fitness (for same seq-number context).

## 6) Rewrite-Ready Snippets (Most Likely Viva Targets)

These are the most probable blocks a teacher can remove and ask you to rewrite.

### 6.1 Fitness function

```cpp
double
RoutingProtocol::CalculateFitness(double residualEnergy, uint8_t hopCount, double velocity) const
{
    double energyRatio = 0.0;
    if (m_initialEnergy > 0.0)
    {
        energyRatio = residualEnergy / m_initialEnergy;
    }

    double hopFactor = 0.0;
    if (hopCount > 0)
    {
        hopFactor = 1.0 / static_cast<double>(hopCount);
    }

    double velocityFactor = 0.0;
    if (m_maxVelocity > 0.0)
    {
        double clampedVelocity = std::min(velocity, m_maxVelocity);
        velocityFactor = (m_maxVelocity - clampedVelocity) / m_maxVelocity;
    }

    return m_alpha * energyRatio + m_beta * hopFactor + m_gamma * velocityFactor;
}
```

### 6.2 RREQ aggregation block (inside `RecvRequest`)

```cpp
double residualEnergy = m_initialEnergy;
Ptr<Node> thisNode = m_ipv4->GetObject<Node>();
Ptr<EnergySourceContainer> energyContainer = thisNode->GetObject<EnergySourceContainer>();
if (energyContainer && energyContainer->GetN() > 0)
{
    residualEnergy = energyContainer->Get(0)->GetRemainingEnergy();
}

double currentSpeed = 0.0;
Ptr<MobilityModel> mobilityModel = thisNode->GetObject<MobilityModel>();
if (mobilityModel)
{
    Vector vel = mobilityModel->GetVelocity();
    currentSpeed = std::sqrt(vel.x * vel.x + vel.y * vel.y + vel.z * vel.z);
}

double localFitness = CalculateFitness(residualEnergy, hop, currentSpeed);
double incomingFitness = rreqHeader.GetPathFitness();
double pathFitness = (incomingFitness == 0.0) ? localFitness
                                               : std::min(incomingFitness, localFitness);
rreqHeader.SetPathFitness(pathFitness);

double incomingVelocity = rreqHeader.GetVelocity();
double pathVelocity = std::max(incomingVelocity, currentSpeed);
rreqHeader.SetVelocity(pathVelocity);
```

### 6.3 Duplicate RREQ improvement rule

```cpp
if (isDuplicateRreq)
{
    RoutingTableEntry existingToOrigin;
    if (m_routingTable.LookupRoute(origin, existingToOrigin))
    {
        if (pathFitness <= existingToOrigin.GetFitness())
        {
            return;
        }
    }
}
```

### 6.4 RREP route-update criterion (fitness beats hop-count tie)

```cpp
else if ((rrepHeader.GetDstSeqno() == toDst.GetSeqNo()) &&
         (replyFitness > toDst.GetFitness()))
{
    m_routingTable.Update(newEntry);
}
```

### 6.5 Packet header field serialization pattern

```cpp
uint64_t fitnessRaw;
std::memcpy(&fitnessRaw, &m_pathFitness, sizeof(double));
i.WriteHtonU64(fitnessRaw);

uint64_t velocityRaw;
std::memcpy(&velocityRaw, &m_velocity, sizeof(double));
i.WriteHtonU64(velocityRaw);
```

Reverse in `Deserialize()` with `ReadNtohU64` and `memcpy` back to double.

## 7) Basic Simulation Mapping (Short, Optional for Presentation)

Even though simulation detail is secondary, this gives context:

1. `scratch/test-ff-aodv.cc`
   - Configures FF-AODV as Phase 1 (`Gamma=0.0`)
   - Prints FlowMonitor metrics + total energy consumed.

2. `scratch/test-ff-aodv-v2.cc`
   - Configures Phase 2 (`Gamma=0.2`, higher mobility)
   - Similar metrics printing.

3. `scratch/experiment-ff-aodv.cc`
   - `protocolVersion=1` => Phase 1 weights
   - `protocolVersion=2` => Phase 2 weights
   - Writes CSV columns including throughput, delay, pdr, drop ratio, energy.

4. `run_experiments.py`
   - Automates multiple runs for both protocol versions and plotting.

## 8) 60-Second Oral Explanation Script

"I modified ns-3 AODV at three levels: control packet headers, route table state, and routing decision logic. First, I added path fitness (and later velocity) fields to RREQ/RREP so each hop can carry and update route quality. Second, I added a fitness field in routing table entries. Third, I replaced hop-based tie breaking with fitness comparison in both RREQ reverse-route creation and RREP forward-route updates. Phase 1 uses energy and hop count only. Phase 2 extends the same pipeline with a velocity penalty and max-velocity tracking along the path. I exposed Alpha/Beta/Gamma/InitialEnergy/MaxVelocity as AODV attributes so experiments can switch Phase 1 and Phase 2 without changing protocol source." 

## 9) Common Teacher Questions You Should Be Ready For

1. Why add fitness to both packet and route table?
   - Packet carries evolving path quality across hops; route table stores best-known quality for future comparisons.

2. Why weakest-link `min()` for path fitness?
   - Path reliability is limited by the worst node/link condition.

3. Why `max()` for path velocity?
   - A single very fast node can destabilize the route, so worst mobility is a useful risk indicator.

4. How does this remain compatible with Phase 1?
   - Set `Gamma=0.0`; velocity term becomes zero.

5. Where is old AODV behavior changed most directly?
   - In `RecvRequest` duplicate handling and reverse-route updates, and in `RecvReply` same-seq route update criterion.

## 10) Quick Pre-Viva Practice Checklist

Before presenting, practice rewriting these from memory:

1. `CalculateFitness` full function
2. RREQ local energy/speed extraction + path aggregation block
3. Duplicate RREQ improve-only condition
4. RREP same-seq fitness comparison block
5. RREQ/RREP serialize/deserialize for double fields

If you can rewrite those five correctly, you can recover most of the protocol changes under pressure.
