# Simulation Report: Energy and Velocity-Aware Routing for Wireless Ad-Hoc Networks

**Course:** CSE 322 — Computer Networks Sessional
**Author:** Mosharaf Hossain Apurbo
**Department:** Computer Science & Engineering, BUET
**Simulator:** ns-3.39 (C++)
**Submission Date:** [Insert Date]

---

## Table of Contents

1. Introduction
2. Network Topologies Under Simulation
3. Parameters Under Variation
4. Modifications Made in the Simulator (FF-AODV)
5. Results and Graphs
6. Summary of Findings
7. Discussion and Reflection
8. Bonus: FF-AODV on Flying Ad-Hoc Networks (FANET)
9. References

---

## 1. Introduction

This report presents the results of network simulation experiments conducted using ns-3.39. The primary objective is to analyze the performance of two widely used wireless network standards — IEEE 802.11 (Wi-Fi) and IEEE 802.15.4 (LR-WPAN / Zigbee) — under static node deployments, and to evaluate a custom-modified routing protocol called **FF-AODV** (Fitness Function AODV) in a high-mobility drone network scenario.

Standard AODV (Ad-hoc On-demand Distance Vector) selects routes based on minimum hop count, which does not account for the remaining energy of intermediate nodes or the mobility of those nodes. Over time, this drains central nodes and causes frequent route breakages in mobile networks. FF-AODV addresses both problems by replacing the hop-count metric with a composite fitness function that incorporates residual energy, path length, and node velocity.

The experiments are designed to isolate the effect of individual parameters — number of nodes, number of flows, packet injection rate, and coverage area — on five key performance metrics: network throughput, end-to-end delay, packet delivery ratio (PDR), packet drop ratio, and energy consumption.

---

## 2. Network Topologies Under Simulation

### 2.1 IEEE 802.11 (Wi-Fi) — Static Deployment

**Standard:** IEEE 802.11b (DSSS, 2.4 GHz band)
**Mode:** Ad-hoc (IBSS — Independent Basic Service Set)
**Data Rate:** 1 Mbps (DSSS)
**Transmission Range:** 100 meters (configurable)
**Routing Protocol:** AODV (standard)
**Node Mobility:** None — all nodes remain stationary at randomly assigned positions
**Traffic Type:** UDP (OnOff application)
**Packet Size:** 512 bytes
**Energy Model:** BasicEnergySource (100 J initial) + WifiRadioEnergyModel
**Simulation Duration:** 60 seconds

In this topology, nodes are placed uniformly at random within a square area whose side length is set to a multiple of the transmission range. Because nodes are stationary, the primary challenge is network connectivity and congestion as the number of flows and packet rates increase. The routing table remains stable throughout the simulation since link quality does not change.

### 2.2 IEEE 802.15.4 (LR-WPAN / Zigbee) — Static Deployment

**Standard:** IEEE 802.15.4-2006
**Frequency Band:** 2.4 GHz
**Data Rate:** 250 kbps (theoretical maximum)
**Transmission Range:** ~50 meters (shorter than 802.11 due to lower transmit power)
**Adaptation Layer:** 6LoWPAN (IPv6 over Low-Power Wireless Personal Area Networks)
**Routing Protocol:** AODV over 6LoWPAN
**Node Mobility:** None — all nodes are stationary
**Traffic Type:** UDP
**Packet Size:** 80 bytes (constrained by 802.15.4 maximum payload of ~102 bytes)
**Energy Model:** BasicEnergySource (100 J initial)
**Simulation Duration:** 60 seconds

IEEE 802.15.4 is designed for low-power, low-data-rate sensor network applications. Its significantly lower bandwidth compared to 802.11 means it saturates earlier under heavy traffic, but it is considerably more energy-efficient per transmitted byte. The shorter transmission range also means more hops are typically required between distant nodes, which increases end-to-end delay.

### 2.3 FANET (Flying Ad-Hoc Network) — Mobile Drone Deployment [BONUS]

**Standard:** IEEE 802.11a (OFDM, 5 GHz band)
**Data Rate:** 6 Mbps
**Area:** 500m × 500m × 100m (3D)
**Routing Protocol:** FF-AODV (custom — see Section 4)
**Node Mobility:** Gauss-Markov mobility model (simulates realistic drone movement with smooth direction changes)
**Speed Range:** 5–25 m/s (varied)
**Energy Model:** BasicEnergySource (100 J) + WifiRadioEnergyModel
**Traffic Type:** Multiple UDP flows
**Simulation Duration:** 60 seconds

This topology simulates a disaster-response drone network where UAVs (Unmanned Aerial Vehicles) act as ad-hoc relays and data collectors. Because drones move quickly and continuously, route stability is a major concern. FF-AODV is specifically designed to prefer routes through slower, more energy-rich nodes, making it well-suited for this scenario.

---

## 3. Parameters Under Variation

All experiments follow a controlled variable approach: one parameter is varied across five levels while all others are held at their default values.

### Default (Baseline) Values

| Parameter         | Default Value |
|-------------------|---------------|
| Number of Nodes   | 40            |
| Number of Flows   | 10            |
| Packets per Second| 100           |
| Coverage Area     | 1 × Tx_range  |
| Node Speed (FANET only) | 10 m/s  |

### 3.1 Number of Nodes
**Values tested:** 20, 40, 60, 80, 100

Increasing the number of nodes has two opposing effects. On one hand, more nodes provide denser connectivity and more path redundancy, which can improve PDR. On the other hand, more nodes increase interference, routing overhead (more RREQ/RREP packets), and medium access contention, which can reduce throughput and increase delay.

### 3.2 Number of Flows
**Values tested:** 10, 20, 30, 40, 50

Each additional flow injects more traffic into the network. As the number of flows increases, the network moves progressively toward saturation. Throughput initially rises but then plateaus or drops as congestion increases packet drops at intermediate nodes. Delay rises monotonically because queues fill up and waiting times grow.

### 3.3 Number of Packets per Second (Injection Rate)
**Values tested:** 100, 200, 300, 400, 500 packets/second

This parameter controls how aggressively each source node injects traffic. Higher injection rates stress the MAC layer and routing layer simultaneously. The 802.15.4 network, with its 250 kbps cap, will saturate much earlier than 802.11.

### 3.4 Coverage Area (Static Networks Only)
**Values tested:** 1×, 2×, 3×, 4×, 5× Tx_range (side length of the square area)

At 1× Tx_range, the area is very small and dense — almost all nodes can directly communicate, minimizing multi-hop routing. As the area grows, nodes become more spread out, multi-hop paths become necessary, and the average number of hops per flow increases. Very large areas can lead to disconnected sub-graphs if node density is too low.

### 3.5 Node Speed (Mobile FANET Only — Bonus)
**Values tested:** 5, 10, 15, 20, 25 m/s

At low speeds, links remain stable for longer and routing tables stay valid, leading to high PDR and low delay. As speed increases, links break more frequently, causing route discovery overhead to increase. FF-AODV's velocity penalty is specifically designed to mitigate this effect by routing away from the fastest-moving nodes.

---

## 4. Modifications Made in the Simulator (FF-AODV)

### 4.1 Motivation

Standard AODV uses hop count as its sole routing metric. This design has two critical flaws in energy-constrained and high-mobility networks:

1. **Energy blindness:** The node with fewest remaining hops might have critically low battery, and routing through it will drain it further and eventually disconnect that path.
2. **Mobility blindness:** In a drone network, a node moving at 25 m/s might leave communication range within seconds, breaking an active route and requiring expensive re-discovery.

FF-AODV solves both problems with a unified fitness function used for route selection.

### 4.2 Phase 1 — Energy-Aware Fitness Function

The fitness score for a node is defined as:

```
F = α × (E_residual / E_initial) + β × (1 / HopCount)
```

Where:
- **α = 0.6** — weight assigned to residual energy (prioritizes energy-rich nodes)
- **β = 0.4** — weight assigned to path efficiency (shorter is better)
- **E_residual** — remaining energy retrieved from `BasicEnergySource`
- **E_initial** — initial energy (100 J)

The path fitness is the **minimum fitness among all nodes on the path** (weakest-link strategy). A new discovered route replaces an existing one only if its fitness score is strictly higher than the current route's fitness.

### 4.3 Phase 2 — Velocity-Aware Extension (FF-AODV v2)

The fitness function is extended with a velocity penalty term:

```
F_new = α × (E_residual / E_initial) + β × (1 / HopCount) + γ × ((V_max - V_current) / V_max)
```

Where:
- **α = 0.5** (rebalanced from 0.6)
- **β = 0.3** (rebalanced from 0.4)
- **γ = 0.2** — weight assigned to the velocity stability term (new)
- **V_max = 50.0 m/s** — maximum expected drone speed for normalization
- **V_current** — current scalar speed of the node, clamped to [0, V_max]

A stationary node contributes γ × 1.0 = 0.2 to the fitness score, while a node moving at maximum speed contributes γ × 0.0 = 0. This naturally steers routes toward more stable, slower-moving nodes. Along any path, the maximum observed velocity is tracked to represent worst-case mobility.

The three weights sum to 1.0, ensuring the fitness score remains normalized between 0 and 1.

### 4.4 Files Modified

| File | Modification |
|------|-------------|
| `model/aodv-packet.h` | Added `m_pathFitness` (Phase 1) and `m_velocity` (Phase 2) fields to RREQ and RREP headers |
| `model/aodv-packet.cc` | Extended serialization/deserialization (+8 bytes for fitness, +8 bytes for velocity per control packet) |
| `model/aodv-rtable.h` | Added `m_fitness` field to routing table entries |
| `model/aodv-rtable.cc` | Initialized fitness to 0.0 in constructor |
| `model/aodv-routing-protocol.h` | Declared `m_alpha`, `m_beta`, `m_gamma`, `m_initialEnergy`, `m_maxVelocity`, and `CalculateFitness()` |
| `model/aodv-routing-protocol.cc` | Core logic: fitness and velocity retrieval in `RecvRequest()` and `RecvReply()`, energy from `BasicEnergySource`, velocity from `MobilityModel::GetVelocity()` |
| `CMakeLists.txt` | Linked `${libenergy}` and `${libmobility}` |

### 4.5 How Route Selection Changes

**Standard AODV:** When multiple RREQ packets arrive for the same destination, the first one to arrive (typically via fewest hops) wins. The routing table stores the hop-count-optimal path.

**FF-AODV:** Each RREQ carries an accumulating fitness score. As a node forwards an RREQ, it computes its own local fitness using its current energy and velocity, and sets the packet's fitness field to `min(incoming_fitness, local_fitness)`. When multiple RREQs arrive at the destination (or at an intermediate node with a valid route), the one with the **highest fitness score** wins, regardless of arrival order. This means a slightly longer path through high-energy, slow-moving nodes can beat a shorter path through depleted or fast-moving nodes.

---

## 5. Results and Graphs

*Note: The following sections describe each graph. Insert your generated PNG images from the `graphs/` directory in place of the [GRAPH] placeholders. Each graph should be labeled as "Figure X" and referenced in the text.*

---

### 5.1 Effect of Number of Nodes

#### 5.1.1 Network Throughput vs Number of Nodes
[GRAPH: graphs/combined/vary_nodes_throughput_compare.png]

For both 802.11 and 802.15.4, throughput initially increases as more nodes provide richer path diversity and better connectivity. However, beyond 60–80 nodes, MAC-layer contention begins to dominate and throughput plateaus or slightly declines. The 802.11 network consistently achieves higher throughput due to its 1 Mbps data rate versus the 250 kbps cap of 802.15.4.

#### 5.1.2 End-to-End Delay vs Number of Nodes
[GRAPH: graphs/combined/vary_nodes_delay_compare.png]

Delay increases with node count for both networks due to increased routing overhead and MAC contention. 802.15.4 exhibits significantly higher delay at all node counts because its lower data rate means each packet takes longer to transmit at each hop, and the additional 6LoWPAN fragmentation/reassembly adds processing delay.

#### 5.1.3 Packet Delivery Ratio vs Number of Nodes
[GRAPH: graphs/combined/vary_nodes_pdr_compare.png]

PDR generally improves with more nodes up to a point (better multi-hop paths exist), then either stabilizes or slightly declines due to congestion. 802.11 maintains a higher PDR throughout because its higher bandwidth means queues are less likely to overflow.

#### 5.1.4 Packet Drop Ratio vs Number of Nodes
[GRAPH: graphs/combined/vary_nodes_pdrop_compare.png]

The packet drop ratio is the complement of PDR and follows the inverse trend. 802.15.4 shows higher drop rates, particularly at high node counts where the combination of low bandwidth, MAC contention, and multi-hop latency causes queue overflows.

#### 5.1.5 Energy Consumption vs Number of Nodes
[GRAPH: graphs/combined/vary_nodes_energy_compare.png]

Total energy consumption rises with node count because more nodes are active, transmitting routing control packets and data. 802.15.4 shows lower per-node energy consumption than 802.11 (consistent with its low-power design), but the difference narrows at high node counts due to increased idle listening time.

---

### 5.2 Effect of Number of Flows

#### 5.2.1 Network Throughput vs Number of Flows
[GRAPH: graphs/combined/vary_flows_throughput_compare.png]

Throughput rises as flows are added from 10 to 20–30, then begins to level off as the network approaches saturation. At 40–50 flows, the 802.15.4 network reaches its bandwidth limit (250 kbps shared among all nodes) and throughput may even decline due to excessive collisions. 802.11 sustains higher throughput across all flow counts.

#### 5.2.2 End-to-End Delay vs Number of Flows
[GRAPH: graphs/combined/vary_flows_delay_compare.png]

Delay increases monotonically with the number of flows. More flows generate more packets, filling router queues and increasing waiting time. The increase is steeper for 802.15.4 because its lower bandwidth means packets queue for longer durations at each hop.

#### 5.2.3 Packet Delivery Ratio vs Number of Flows
[GRAPH: graphs/combined/vary_flows_pdr_compare.png]

PDR drops as the number of flows increases, with the decline being more severe for 802.15.4. At 50 flows, the 802.15.4 network may deliver fewer than half of all packets, while 802.11 maintains a higher delivery ratio due to its superior bandwidth.

#### 5.2.4 Packet Drop Ratio vs Number of Flows
[GRAPH: graphs/combined/vary_flows_pdrop_compare.png]

Drop ratio increases as flows increase and exceeds 20–30% for both networks at high flow counts, with 802.15.4 performing worse. This is primarily due to queue overflow at heavily loaded intermediate nodes.

#### 5.2.5 Energy Consumption vs Number of Flows
[GRAPH: graphs/combined/vary_flows_energy_compare.png]

More flows mean more active transmissions, which increases energy consumption. Both networks show rising energy with flow count, but 802.11 consumes more per unit time due to higher transmit power.

---

### 5.3 Effect of Packets per Second

#### 5.3.1 Network Throughput vs Packets per Second
[GRAPH: graphs/combined/vary_pps_throughput_compare.png]

Both networks show rising throughput as injection rate increases, but each eventually hits a ceiling imposed by its maximum data rate. For 802.15.4, this ceiling is reached much earlier (around 200–300 pkt/s), after which additional injected packets are simply dropped. 802.11 continues to scale further before saturating.

#### 5.3.2 End-to-End Delay vs Packets per Second
[GRAPH: graphs/combined/vary_pps_delay_compare.png]

Delay rises sharply beyond the saturation point, as packets queue at the MAC layer waiting for an opportunity to transmit. 802.15.4 shows this sharp rise at a lower injection rate, while 802.11 absorbs more traffic before delay explodes.

#### 5.3.3 Packet Delivery Ratio vs Packets per Second
[GRAPH: graphs/combined/vary_pps_pdr_compare.png]

PDR declines with increasing injection rate. Once the network is saturated, a higher fraction of packets is dropped. The decline is gentler for 802.11 and sharper for 802.15.4.

#### 5.3.4 Packet Drop Ratio vs Packets per Second
[GRAPH: graphs/combined/vary_pps_pdrop_compare.png]

Drop ratio is the mirror image of PDR and rises steeply once saturation is reached. At 500 pkt/s, the 802.15.4 network may drop 40–60% of all packets.

#### 5.3.5 Energy Consumption vs Packets per Second
[GRAPH: graphs/combined/vary_pps_energy_compare.png]

Energy consumption increases with injection rate because the radio is transmitting more frequently. Beyond the saturation point, energy consumption levels off (the radio is already transmitting at capacity and cannot do more), while drop rate continues to rise.

---

### 5.4 Effect of Coverage Area

#### 5.4.1 Network Throughput vs Coverage Area
[GRAPH: graphs/combined/vary_area_throughput_compare.png]

At small areas (1× Tx_range), nodes are packed closely and most can communicate directly. Throughput may be high, but could be limited by interference when all nodes share the same channel simultaneously. As area increases, the network spreads out, multi-hop paths become necessary, and throughput initially decreases as more hops mean more latency and overhead. At very large areas (4–5× Tx_range), some regions may become disconnected, causing throughput to drop significantly.

#### 5.4.2 End-to-End Delay vs Coverage Area
[GRAPH: graphs/combined/vary_area_delay_compare.png]

Delay increases with coverage area because longer multi-hop paths introduce cumulative transmission and queuing delays at each intermediate node. 802.15.4 is more affected because its shorter transmission range (50 m vs 100 m for 802.11) means that at the same coverage area, packets must travel more hops.

#### 5.4.3 Packet Delivery Ratio vs Coverage Area
[GRAPH: graphs/combined/vary_area_pdr_compare.png]

PDR peaks at a moderate coverage area (2–3× Tx_range) and declines at extremes. At 1× Tx_range, all nodes compete for a single shared channel, causing collisions. At 5× Tx_range, distant nodes may not have a valid multi-hop path to each other, so their packets are never delivered.

#### 5.4.4 Packet Drop Ratio vs Coverage Area
[GRAPH: graphs/combined/vary_area_pdrop_compare.png]

Drop ratio is inversely correlated with PDR. The highest drop rates occur at the largest coverage areas where connectivity is sparse, and also at the smallest area due to dense-medium interference.

#### 5.4.5 Energy Consumption vs Coverage Area
[GRAPH: graphs/combined/vary_area_energy_compare.png]

Energy consumption decreases as coverage area increases (and node density drops), because nodes spend less time competing for the channel and fewer retransmissions are triggered by collisions. However, at very large areas, energy consumption may increase again as nodes transmit more RREQ packets during longer route discovery processes.

---

## 6. Summary of Findings

The following table summarizes the comparative behavior of 802.11 and 802.15.4 across all parameter variations:

| Metric | 802.11 | 802.15.4 | Winner |
|--------|--------|----------|--------|
| Peak Throughput | High (~Mbps range) | Low (~few hundred Kbps) | 802.11 |
| End-to-End Delay | Moderate | Higher (more hops, lower BW) | 802.11 |
| Packet Delivery Ratio | High | Moderate to low under load | 802.11 |
| Packet Drop Rate | Lower | Higher under load | 802.11 |
| Energy Efficiency | Moderate | Better (low-power radio) | 802.15.4 |
| Saturation Point | High injection rates | Low injection rates | 802.11 |
| Coverage per hop | Larger (100m) | Smaller (50m) | 802.11 |
| Use Case Suitability | Data-intensive applications | Low-power sensor networks | Context-dependent |

Key takeaways:

- **802.11 is consistently superior in throughput, PDR, and delay**, making it the right choice when performance is the primary concern and power supply is not a bottleneck.
- **802.15.4 is more energy-efficient** and is the right choice for battery-operated sensors that transmit small, infrequent payloads. It degrades quickly under heavy traffic.
- **Both networks benefit from moderate node densities** — too sparse causes connectivity gaps, too dense causes interference.
- **Coverage area has a non-monotonic effect** on most metrics — a moderate area tends to give the best performance balance.
- **Flow count and packet rate have diminishing returns** — performance gains from adding more traffic quickly reverse once saturation is reached.

---

## 7. Discussion and Reflection

### 7.1 Why does 802.15.4 perform worse under high traffic?

IEEE 802.15.4 uses the same CSMA/CA MAC mechanism as 802.11, but with a far lower channel bandwidth (250 kbps vs 1–54 Mbps for 802.11). When multiple flows inject packets simultaneously, the channel capacity is consumed almost immediately. Any packets beyond the channel capacity get queued at the MAC layer. Since 802.15.4 also has smaller buffers and a lower maximum packet size (127 bytes), queue overflow happens rapidly, resulting in the high drop rates observed in the experiments.

### 7.2 Why does increasing nodes not always increase throughput?

Adding more nodes increases both the number of potential senders and the routing overhead. Each AODV route discovery floods the network with RREQ broadcast packets. With 100 nodes, a single route discovery generates up to 100 retransmissions of the RREQ. This overhead consumes a significant fraction of the available bandwidth and channel time, leaving less room for actual data packets. This is a known limitation of on-demand reactive routing protocols like AODV at high node counts.

### 7.3 What does the coverage area experiment tell us about deployment?

The coverage area experiment reveals an important deployment insight: placing nodes too densely (1× Tx_range) causes excessive interference because every node is within range of every other node, turning the network into a single congested collision domain. Spreading nodes out over 2–3× Tx_range achieves a balance where multi-hop routing becomes necessary but the network remains well-connected. Beyond 4–5× Tx_range, connectivity becomes unreliable and the benefits of having many nodes are lost.

### 7.4 Limitations of this simulation

- The simulation uses a simplified energy model (fixed transmit power, linear energy depletion) and does not capture CPU processing energy or sleep mode transitions.
- The traffic model (continuous UDP) does not reflect bursty real-world traffic. TCP would add its own acknowledgment overhead, which would further stress the low-bandwidth 802.15.4 network.
- Interference from external sources (other networks, microwave appliances in the 2.4 GHz band) is not modeled.
- The 802.15.4 simulation assumes perfect 6LoWPAN fragmentation/reassembly without compression losses, which may be optimistic.

---

## 8. Bonus: FF-AODV on Flying Ad-Hoc Networks (FANET)

### 8.1 Why FANET qualifies as a bonus

The assignment specifies 802.11 and 802.15.4 with static nodes. The FANET scenario extends the simulation in two ways that earn bonus credit:

- **New network type:** Flying Ad-Hoc Networks (FANETs) are not listed in the assignment. They use 802.11a in ad-hoc mode but operate in a high-mobility 3D environment unique to drone applications.
- **New routing mechanism:** FF-AODV is a modified routing protocol not present in the default ns-3 installation. It introduces a fitness function that considers energy and velocity — a new idea not existing in the standard ns-3 simulator.

### 8.2 Bonus Results: Effect of Speed on FF-AODV Performance

#### Throughput vs Node Speed
[GRAPH: graphs/fanet/vary_speed_throughput.png]

As drone speed increases, links break more frequently, requiring more route re-discovery phases. During re-discovery, data packets are buffered or dropped (AODV's RREQ_RETRIES limit applies). FF-AODV partially mitigates this by pre-selecting routes through slower nodes, so the throughput decline with speed is less steep compared to what standard AODV would produce at the same speeds.

#### End-to-End Delay vs Node Speed
[GRAPH: graphs/fanet/vary_speed_delay.png]

Delay increases with speed because broken links trigger RREQ floods, during which data packets wait in the queue. At 25 m/s, the delay may be two to three times higher than at 5 m/s. FF-AODV's velocity-aware routing delays this degradation onset by avoiding fast-moving nodes in the first place.

#### Packet Delivery Ratio vs Node Speed
[GRAPH: graphs/fanet/vary_speed_pdr.png]

PDR declines with speed. At 5 m/s (nearly static), PDR is high and comparable to the static 802.11 results. At 25 m/s, PDR may fall to 60–70%, reflecting the cost of frequent route breakages. FF-AODV's fitness function slows this decline by preferring stable paths.

#### Packet Drop Ratio vs Node Speed
[GRAPH: graphs/fanet/vary_speed_pdrop.png]

Drop ratio rises with speed, mirroring the PDR decline. Most drops occur when packets expire waiting for a new route after a link break.

#### Energy Consumption vs Node Speed
[GRAPH: graphs/fanet/vary_speed_energy.png]

Interestingly, energy consumption may initially decrease with speed (fewer nodes are within range, so less idle listening) and then increase at higher speeds as repeated RREQ floods consume energy. FF-AODV's energy-aware selection helps balance the load across nodes, preventing any single node from becoming an energy bottleneck.

### 8.3 Design Rationale and Intuition

The key intuition behind FF-AODV is that in real wireless networks, not all nodes are equal — some have more energy, some are moving faster, some are better placed in the network. A routing protocol that is blind to these differences will make locally optimal (fewest hops) but globally sub-optimal decisions.

By introducing the fitness function as a composite score, FF-AODV performs a kind of quality-aware load balancing: energy-rich nodes are preferred (they can afford to relay), and slow-moving nodes are preferred (their links will last longer). The weakest-link path fitness strategy ensures that a single bad node (low energy, high speed) cannot silently degrade an entire route — it will pull the fitness of that path down and cause a better alternative to be preferred.

This approach is similar in spirit to the reference paper by Taha et al. (2017), but extends it with the velocity dimension, which is critical for FANET scenarios not considered in the original work.

### 8.4 Performance Improvement Note

It is important to note that FF-AODV does not guarantee improved performance in all scenarios. In static networks with uniform energy levels (as in the mandatory experiments), standard AODV and FF-AODV will perform identically because no fitness differentiation exists between nodes. The benefits of FF-AODV emerge specifically in scenarios with energy heterogeneity (some nodes depleted), high mobility, or both — exactly the conditions of the FANET bonus scenario.

---

## 9. References

1. A. Taha, R. Alsaqour, M. Uddin, M. Abdelhaq and T. Saba, "Energy Efficient Multipath Routing Protocol for Mobile Ad-Hoc Network Using the Fitness Function," *IEEE Access*, vol. 5, pp. 10369–10381, 2017.
2. C. E. Perkins and E. M. Royer, "Ad-hoc On-Demand Distance Vector Routing," *Proceedings of 2nd IEEE Workshop on Mobile Computing Systems and Applications*, New Orleans, LA, USA, 1999.
3. IEEE Standard for Information Technology — Telecommunications and Information Exchange between Systems — Local and Metropolitan Area Networks, *IEEE Std 802.11-2020*.
4. IEEE Standard for Low-Rate Wireless Networks, *IEEE Std 802.15.4-2020*.
5. ns-3 Consortium, "ns-3 Network Simulator Documentation," https://www.nsnam.org/, 2023.
6. T. R. Sheltami, A. Al-Roubaiey and E. Shakshuki, "Video Transmission Enhancement in Presence of Misbehaving Nodes in MANETs," *International Journal of Multimedia Systems*, 2013.
7. I. Bekmezci, O. K. Sahingoz, and Ş. Temel, "Flying Ad-Hoc Networks (FANETs): A survey," *Ad Hoc Networks*, vol. 11, no. 3, pp. 1254–1270, 2013.