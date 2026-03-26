# FF-AODV: Energy & Velocity-Aware Routing for Disaster Response Drone Networks

**Author:** Mosharaf Hossain Apurbo  
**Department:** Computer Science & Engineering, BUET  
**Course:** CSE 322 — Computer Networks Sessional  
**Simulator:** ns-3.39 (C++)  
**Reference Paper:** A. Taha et al., *"Energy Efficient Multipath Routing Protocol for Mobile Ad-Hoc Network Using the Fitness Function,"* IEEE Access, vol. 5, 2017.

---

## Project Goal

Implement an energy- and velocity-aware modification of the AODV routing protocol (called **FF-AODV**) for Flying Ad-Hoc Networks (FANETs) in a disaster response scenario. Standard AODV picks the shortest hop-count path, which drains central nodes and ignores link instability from fast-moving drones. FF-AODV replaces hop-count-based route selection with a **fitness function** that balances residual energy, path length, and node velocity.

---

## Current Status

| Phase | Description | Status |
|-------|-------------|--------|
| **Phase 1** | Paper Implementation (Energy-Aware) | **DONE** |
| **Phase 2** | Modification (Velocity-Aware) | **DONE** |

---

## What Was Done (Phase 1)

### Core Algorithm Change

Replaced AODV's route selection metric from `hop count` to a **fitness score**:

$$F = \alpha \times \frac{E_{residual}}{E_{initial}} + \beta \times \frac{1}{HopCount}$$

- **α = 0.6** (energy weight), **β = 0.4** (distance weight)  *(Phase 1 defaults)*
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

### Test Simulation (Phase 1)

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

## What Was Done (Phase 2 — Velocity-Aware Extension)

### Motivation

In high-mobility FANET scenarios (drones moving 20–50 m/s), Phase 1's fitness function only considers energy and hop count. Fast-moving nodes cause frequent link breaks, leading to route instability and reduced Packet Delivery Ratio (PDR). Phase 2 adds a **velocity penalty** to the fitness function so that routes preferentially go through slower-moving (more stable) nodes.

### Extended Fitness Function

$$F_{new} = \alpha \times \frac{E_{residual}}{E_{initial}} + \beta \times \frac{1}{HopCount} + \gamma \times \frac{V_{max} - V_{current}}{V_{max}}$$

| Parameter | Value | Description |
|-----------|-------|-------------|
| **α** | 0.5 | Energy weight (rebalanced from 0.6) |
| **β** | 0.3 | Hop count weight (rebalanced from 0.4) |
| **γ** | 0.2 | Velocity weight (new) |
| **V_max** | 50.0 m/s | Maximum expected velocity for normalization |

**Key design choices:**
- Weights sum to 1.0 (α + β + γ = 0.5 + 0.3 + 0.2 = 1.0)
- Velocity is clamped to [0, V_max] to avoid negative fitness contributions
- **Slower nodes score higher** — a stationary node contributes γ×1.0, while a node at V_max contributes γ×0.0
- The **maximum velocity** along the path is tracked (worst-case mobility indicator)

### Files Modified (Phase 2)

| File | Phase 2 Change |
|------|----------------|
| `model/aodv-packet.h` | Added `m_velocity` field with `SetVelocity()`/`GetVelocity()` to both `RreqHeader` and `RrepHeader` |
| `model/aodv-packet.cc` | Serialization/deserialization of velocity (+8 bytes per RREQ/RREP). RREQ now 39 bytes (was 31), RREP now 35 bytes (was 27). Updated `operator==` and `Print()` |
| `model/aodv-routing-protocol.h` | Added `#include "ns3/mobility-model.h"`, `m_gamma` (0.2), `m_maxVelocity` (50.0). Updated `CalculateFitness()` signature to accept velocity parameter |
| `model/aodv-routing-protocol.cc` | **Constructor:** initialized `m_gamma(0.2)`, `m_maxVelocity(50.0)`, rebalanced weights. **GetTypeId():** registered `Gamma` and `MaxVelocity` as ns-3 attributes. **CalculateFitness():** added velocity penalty term. **RecvRequest():** retrieves velocity via `MobilityModel::GetVelocity()`, computes speed magnitude, sets max velocity on RREQ, passes to fitness. **RecvReply():** same velocity integration pattern |

### How Velocity Integration Works

1. **When a node receives an RREQ or RREP**, it:
   - Retrieves its current velocity vector from `MobilityModel::GetVelocity()`
   - Computes the scalar speed: `speed = √(vx² + vy² + vz²)`
   - Computes its local fitness including the velocity term
   - Updates the packet's velocity field to the **maximum** of (incoming velocity, local speed)
   - Takes the **minimum** fitness along the path (weakest-link)

2. **Route selection** uses the same fitness comparison as Phase 1:
   - New route accepted only if `F_new > F_old`
   - This naturally penalizes paths through fast-moving nodes

### Test Simulation (Phase 2)

File: `scratch/test-ff-aodv-v2.cc`

| Parameter | Value |
|-----------|-------|
| Nodes | 15 UAVs |
| Wi-Fi | 802.11a ad-hoc, 6 Mbps |
| Area | 500m x 500m |
| Mobility | Gauss-Markov (**20–50 m/s** — high mobility) |
| Energy | 100 J per node |
| Routing | FF-AODV v2 (α=0.5, β=0.3, γ=0.2, V_max=50) |
| Traffic | 2 UDP flows: Node 0→14, Node 1→13, 64 kbps each |
| Duration | 60 seconds |
| Metrics | FlowMonitor, energy stats, velocity snapshots |

---

## How to Build & Run

```bash
cd ns-allinone-3.39/ns-3.39
./ns3 configure --enable-examples --enable-tests
./ns3 build

# Run Phase 1 test (low-mobility, 10 nodes)
./ns3 run "test-ff-aodv"

# Run Phase 2 test (high-mobility, 15 nodes)
./ns3 run "test-ff-aodv-v2"
```

### Configurable Parameters (via ns-3 attributes)

```cpp
AodvHelper aodv;
aodv.Set("Alpha", DoubleValue(0.5));         // energy weight
aodv.Set("Beta", DoubleValue(0.3));          // hop count weight
aodv.Set("Gamma", DoubleValue(0.2));         // velocity weight
aodv.Set("InitialEnergy", DoubleValue(100.0)); // Joules
aodv.Set("MaxVelocity", DoubleValue(50.0));  // m/s
```

---

## Project Structure

```
ns-3.39/
├── src/aodv/
│   ├── CMakeLists.txt                    # Links ${libenergy}
│   └── model/
│       ├── aodv-packet.h                 # RREQ/RREP with fitness + velocity
│       ├── aodv-packet.cc                # Serialization (+16 bytes total)
│       ├── aodv-rtable.h                 # Route entry with fitness score
│       ├── aodv-rtable.cc                # Fitness initialization
│       ├── aodv-routing-protocol.h       # α, β, γ, Vmax, CalculateFitness()
│       └── aodv-routing-protocol.cc      # Core FF-AODV v2 logic
└── scratch/
    ├── test-ff-aodv.cc                   # Phase 1 test (low mobility)
    └── test-ff-aodv-v2.cc               # Phase 2 test (high mobility)
```

---

## References

1. A. Taha, R. Alsaqour, M. Uddin, M. Abdelhaq and T. Saba, "Energy Efficient Multipath Routing Protocol for Mobile Ad-Hoc Network Using the Fitness Function," IEEE Access, vol. 5, pp. 10369–10381, 2017.
2. ns-3 Consortium, "ns-3 Network Simulator," https://www.nsnam.org/
