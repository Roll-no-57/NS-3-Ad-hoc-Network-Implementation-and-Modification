# FF-AODV: Energy-Aware Routing for Disaster Response Drone Networks

**Author:** Mosharaf Hossain Apurbo  
**Department:** Computer Science & Engineering, BUET  
**Course:** CSE 322 — Computer Networks Sessional  
**Simulator:** ns-3.39 (C++)  
**Reference Paper:** A. Taha et al., *"Energy Efficient Multipath Routing Protocol for Mobile Ad-Hoc Network Using the Fitness Function,"* IEEE Access, vol. 5, 2017.

---

## Project Goal

Implement an energy-aware modification of the AODV routing protocol (called **FF-AODV**) for Flying Ad-Hoc Networks (FANETs) in a disaster response scenario. Standard AODV picks the shortest hop-count path, which drains central nodes and causes premature network failure. FF-AODV replaces hop-count-based route selection with a **fitness function** that balances residual energy and path length.

---

## Current Status

| Phase | Description | Status |
|-------|-------------|--------|
| **Phase 1** | Paper Implementation | **DONE** |
| **Phase 2** | Modification | Not started |

---

## What Was Done (Phase 1)

### Core Algorithm Change

Replaced AODV's route selection metric from `hop count` to a **fitness score**:

$$F = \alpha \times \frac{E_{residual}}{E_{initial}} + \beta \times \frac{1}{HopCount}$$

- **α = 0.6** (energy weight), **β = 0.4** (distance weight)
- Path fitness = **minimum** fitness among all nodes on the path (weakest-link strategy)
- Route updated only if **F_new > F_old** (instead of fewer hops)

### Files Modified (under `ns-3.39/src/aodv/`)

| File | Change |
|------|--------|
| `model/aodv-packet.h` | Added `m_pathFitness` field to RREQ and RREP headers |
| `model/aodv-packet.cc` | Serialization/deserialization of the fitness field (+8 bytes per packet) |
| `model/aodv-rtable.h` | Added `m_fitness` field to routing table entries |
| `model/aodv-rtable.cc` | Initialized fitness to 0.0 in constructor |
| `model/aodv-routing-protocol.h` | Added `m_alpha`, `m_beta`, `m_initialEnergy`, `CalculateFitness()` |
| `model/aodv-routing-protocol.cc` | Core logic: fitness computation in `RecvRequest()` and `RecvReply()`, energy retrieval from `BasicEnergySource` |
| `CMakeLists.txt` | Linked `${libenergy}` dependency |

### Test Simulation Created

File: `scratch/test-ff-aodv.cc`

| Parameter | Value |
|-----------|-------|
| Nodes | 10 UAVs |
| Wi-Fi | 802.11a ad-hoc, 6 Mbps |
| Area | 200m x 200m |
| Mobility | Gauss-Markov (5–20 m/s) |
| Energy | 100 J per node (BasicEnergySource + WifiRadioEnergyModel) |
| Routing | FF-AODV (α=0.6, β=0.4) |
| Traffic | UDP OnOff, Node 0 → Node 9, 64 kbps, 512-byte packets |
| Duration | 30 seconds |
| Metrics | FlowMonitor (PDR, delay, throughput) |

---

## Output / Results

### Build
- **Zero compilation errors.** All AODV module files compile and link successfully.

### Simulation Output

**Energy consumption** — all nodes consumed energy (100 J → ~75.4 J), confirming the energy model is functional and `GetRemainingEnergy()` works inside the routing protocol.

**Flow statistics (sample):**

| Flow | PDR | Avg Delay |
|------|-----|-----------|
| Flow 2 | 92.3% | 141 ms |
| Flow 3 | 66.7% | — |
| Flow 6 | 100% | — |
| Flow 7 | 100% | — |

- Simulation ran to completion with no crashes or assertion failures.
- AODV route discovery (RREQ/RREP) operated correctly with the fitness extension.
- Packets were successfully delivered across the ad-hoc network.

---

## How to Build & Run

```bash
cd ns-allinone-3.39/ns-3.39
./ns3 configure
./ns3 build
./ns3 run "test-ff-aodv"
```

---

## What Remains (Phase 2 — Velocity-Aware)

Extend the fitness function to penalize fast-moving nodes:

$$F_{new} = F_{base} + \gamma \times \frac{V_{max} - V_{current}}{V_{max}}$$

- Add `m_velocity` to RREQ/RREP headers
- Access `MobilityModel::GetVelocity()` during route decisions
- Add **γ = 0.2** as a configurable weight
- Goal: improve PDR in high-mobility (20–50 m/s) scenarios

---

## References

1. A. Taha, R. Alsaqour, M. Uddin, M. Abdelhaq and T. Saba, "Energy Efficient Multipath Routing Protocol for Mobile Ad-Hoc Network Using the Fitness Function," IEEE Access, vol. 5, pp. 10369–10381, 2017.
2. ns-3 Consortium, "ns-3 Network Simulator," https://www.nsnam.org/
3. Detailed implementation docs: `ns-3.39/FF-AODV-README.md`
