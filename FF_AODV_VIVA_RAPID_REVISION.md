# FF-AODV Viva Rapid Revision (1-Page)

Use this as a last-minute memory sheet before presentation.

## 1) 60-Second Pitch

I modified ns-3 AODV in three layers: packet headers, routing table state, and route update logic.
In Phase 1, I replaced hop-only path quality with a fitness function using residual energy and hop count.
In Phase 2, I extended the same pipeline with a velocity term so fast-moving nodes are penalized.
I added attributes Alpha, Beta, Gamma, InitialEnergy, and MaxVelocity so the same code can run both phases by changing parameters.

## 2) Core Formulas

Phase 1:

F = alpha * (E_residual / E_initial) + beta * (1 / hopCount)

Phase 2:

F = alpha * (E_residual / E_initial) + beta * (1 / hopCount) + gamma * ((Vmax - V) / Vmax)

Defaults used:
- Phase 1 mode: alpha=0.6, beta=0.4, gamma=0.0
- Phase 2 mode: alpha=0.5, beta=0.3, gamma=0.2, Vmax=50.0

## 3) Exact Files You Must Remember

Core protocol edits:
- src/aodv/CMakeLists.txt
- src/aodv/model/aodv-packet.h
- src/aodv/model/aodv-packet.cc
- src/aodv/model/aodv-rtable.h
- src/aodv/model/aodv-rtable.cc
- src/aodv/model/aodv-routing-protocol.h
- src/aodv/model/aodv-routing-protocol.cc

Simulation-side config proof:
- scratch/test-ff-aodv.cc
- scratch/test-ff-aodv-v2.cc
- scratch/experiment-ff-aodv.cc

## 4) Critical Logic Memory Triggers

RREQ path aggregation in RecvRequest:
- read local residual energy from EnergySourceContainer
- read local speed from MobilityModel::GetVelocity()
- localFitness = CalculateFitness(...)
- pathFitness = min(incomingFitness, localFitness)
- pathVelocity = max(incomingVelocity, currentSpeed)
- duplicate RREQ only accepted if fitness improves

RREP path aggregation in RecvReply:
- repeat local energy and speed extraction
- replyFitness = min(incomingFitness, localFitness)
- pathVelocity = max(incomingVelocity, currentSpeed)
- same-seq route update uses higher fitness, not lower hop count

## 5) What Changed vs Original AODV

Original behavior:
- duplicates usually dropped early
- route quality heavily tied to seq number freshness + hop count

FF-AODV behavior:
- packet carries path metric fields
- route table stores fitness per destination
- route updates are fitness-improvement driven in key tie cases

## 6) The 5 Most Likely Rewrite Targets

1. CalculateFitness function body
2. RREQ aggregation block in RecvRequest
3. Duplicate RREQ improvement gate
4. RREP same-seq update criterion
5. RREQ/RREP double serialization and deserialization

## 7) Fast Answers for Common Teacher Questions

Q: Why store fitness in both packet and route table?
A: Packet carries hop-by-hop evolving metric; route table stores current best metric for later comparisons.

Q: Why min for pathFitness?
A: Weakest-link policy: path quality is bounded by worst node contribution.

Q: Why max for velocity?
A: One very fast node can destabilize the path; max speed is a worst-case mobility indicator.

Q: How do you switch between Phase 1 and Phase 2?
A: Same codebase, change Gamma and weights via AODV attributes.

Q: Where did you directly replace hop-count decision?
A: In RecvReply same-seq update case and in RecvRequest fitness-based reverse route update logic.

## 8) If You Blank Out During Viva

Rebuild from this order:
1. add fields in packet headers
2. serialize/deserialize fields
3. add route-table fitness member
4. add protocol members and attributes
5. implement CalculateFitness
6. wire RecvRequest and RecvReply aggregation
7. change update comparisons to fitness improvement
