# FF-AODV Rewrite Drill Scenarios

Purpose: simulate teacher removing blocks and asking you to rewrite from memory.

How to use:
1. Open the named file.
2. Hide the target block.
3. Rewrite only from prompt.
4. Compare against acceptance checklist.

## Drill 1: Fitness Function

Location:
- src/aodv/model/aodv-routing-protocol.cc
- function: RoutingProtocol::CalculateFitness

Prompt:
Write the full Phase 2 fitness function with guards and velocity clamp.

Must include:
- energyRatio guarded by initialEnergy > 0
- hopFactor guarded by hopCount > 0
- velocityFactor guarded by maxVelocity > 0
- clamped velocity via min(velocity, maxVelocity)
- weighted return with alpha, beta, gamma

## Drill 2: RREQ Local Aggregation

Location:
- src/aodv/model/aodv-routing-protocol.cc
- function: RecvRequest

Prompt:
Rewrite the block that computes local residual energy and local speed, then updates path fitness and path velocity in RREQ.

Must include:
- EnergySourceContainer read from current node
- MobilityModel speed magnitude calculation
- localFitness using CalculateFitness(residualEnergy, hop, currentSpeed)
- pathFitness = min(incomingFitness, localFitness) with zero fallback
- pathVelocity = max(incomingVelocity, currentSpeed)
- SetPathFitness and SetVelocity on rreqHeader

## Drill 3: Duplicate RREQ Improvement Gate

Location:
- src/aodv/model/aodv-routing-protocol.cc
- function: RecvRequest

Prompt:
Rewrite duplicate RREQ logic so duplicates are dropped only if non-improving fitness.

Must include:
- check isDuplicateRreq
- lookup existing route to origin
- if pathFitness <= existingFitness then return
- otherwise continue processing

## Drill 4: Reverse Route Creation with Fitness

Location:
- src/aodv/model/aodv-routing-protocol.cc
- function: RecvRequest

Prompt:
Rewrite new reverse-route creation path and store fitness.

Must include:
- RoutingTableEntry constructor fields for origin reverse route
- SetFitness(pathFitness)
- AddRoute(newEntry)

## Drill 5: Reverse Route Update with Fitness Improvement

Location:
- src/aodv/model/aodv-routing-protocol.cc
- function: RecvRequest

Prompt:
Rewrite existing reverse-route update branch where route is updated only if fitness improves.

Must include:
- condition pathFitness > toOrigin.GetFitness()
- update nextHop, device, interface, hop, lifetime
- SetFitness(pathFitness)
- m_routingTable.Update(toOrigin)

## Drill 6: SendReply FF-AODV Seeding

Location:
- src/aodv/model/aodv-routing-protocol.cc
- function: SendReply

Prompt:
Rewrite logic that seeds RREP fitness and velocity from RREQ, including backward-compatible fallback.

Must include:
- replyFitness from rreqHeader.GetPathFitness()
- replyVelocity from rreqHeader.GetVelocity()
- fallback recompute if either is zero
- rrepHeader.SetPathFitness(replyFitness)
- rrepHeader.SetVelocity(replyVelocity)

## Drill 7: RREP Local Aggregation

Location:
- src/aodv/model/aodv-routing-protocol.cc
- function: RecvReply

Prompt:
Rewrite local energy/speed extraction and path update in RREP processing.

Must include:
- localFitness computation
- replyFitness min aggregation with zero fallback
- rrepHeader.SetPathFitness(replyFitness)
- pathVelocity max aggregation
- rrepHeader.SetVelocity(pathVelocity)

## Drill 8: Forward Route Entry Fitness Store

Location:
- src/aodv/model/aodv-routing-protocol.cc
- function: RecvReply

Prompt:
Rewrite forward-route newEntry creation and fitness assignment.

Must include:
- new RoutingTableEntry for destination
- newEntry.SetFitness(replyFitness)
- add or update route accordingly

## Drill 9: Same-Sequence Update Criterion

Location:
- src/aodv/model/aodv-routing-protocol.cc
- function: RecvReply

Prompt:
Rewrite the exact same-sequence route update case that compares fitness instead of hop count.

Must include:
- check rrepHeader.GetDstSeqno() == toDst.GetSeqNo()
- compare replyFitness > toDst.GetFitness()
- m_routingTable.Update(newEntry)

## Drill 10: Packet Header Serialization for Double Fields

Location:
- src/aodv/model/aodv-packet.cc
- functions: RreqHeader::Serialize/Deserialize and RrepHeader::Serialize/Deserialize

Prompt:
Rewrite one complete serialize+deserialize pair for pathFitness and velocity.

Must include:
- memcpy double -> uint64_t then WriteHtonU64
- ReadNtohU64 then memcpy uint64_t -> double
- both fields covered: fitness and velocity

## Drill 11: Packet Header API and Members

Location:
- src/aodv/model/aodv-packet.h

Prompt:
Rewrite API additions and private fields for both RreqHeader and RrepHeader.

Must include:
- SetPathFitness / GetPathFitness
- SetVelocity / GetVelocity
- private double m_pathFitness
- private double m_velocity

## Drill 12: Routing Table Fitness State

Location:
- src/aodv/model/aodv-rtable.h and src/aodv/model/aodv-rtable.cc

Prompt:
Rewrite route-table fitness member additions and constructor initialization.

Must include:
- SetFitness / GetFitness in RoutingTableEntry
- m_fitness field in private section
- m_fitness(0.0) in RoutingTableEntry constructor init list

## Drill 13: TypeId Attribute Registration

Location:
- src/aodv/model/aodv-routing-protocol.cc
- function: RoutingProtocol::GetTypeId

Prompt:
Rewrite AddAttribute blocks for Alpha, Beta, InitialEnergy, Gamma, MaxVelocity.

Must include:
- correct names and descriptions
- MakeDoubleAccessor targets
- checker ranges

## Drill 14: Constructor Defaults

Location:
- src/aodv/model/aodv-routing-protocol.cc
- constructor init list

Prompt:
Rewrite FF-AODV relevant constructor defaults.

Must include:
- m_alpha(0.5)
- m_beta(0.3)
- m_initialEnergy(100.0)
- m_gamma(0.2)
- m_maxVelocity(50.0)

## Drill 15: Build Dependency

Location:
- src/aodv/CMakeLists.txt

Prompt:
Rewrite the library linkage change required for energy integration.

Must include:
- ${libenergy} in LIBRARIES_TO_LINK

## Fast Self-Check Rubric (Per Drill)

Give yourself:
- 2 points: all critical lines and logic order correct
- 1 point: minor naming or ordering errors
- 0 points: major missing logic

Score interpretation for 15 drills:
- 24 to 30: viva-ready
- 18 to 23: acceptable but revise weak drills
- below 18: rehearse again before presentation
