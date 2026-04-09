# FF-AODV Mock Viva Q and A Pack

This file is for oral practice.
Use it in 3 rounds: quick theory, code-level depth, and blackboard defense.

Scoring suggestion:
- 2 points: correct + clear + confident
- 1 point: partially correct
- 0 points: wrong or unclear

Target:
- Round 1 (20 points): 14+
- Round 2 (30 points): 20+
- Round 3 (20 points): 14+

Total target: 48/70 or higher.

## Round 1: Quick Fire (10 Questions)

1. What problem in standard AODV motivated your work?
2. What is the exact difference between Phase 1 and Phase 2 objective terms?
3. Why does your code add fields to both RREQ and RREP headers?
4. Why is libenergy linked in src/aodv/CMakeLists.txt?
5. What values are used for alpha, beta, gamma in Phase 2?
6. How do you compute scalar speed from MobilityModel velocity?
7. Why use min for pathFitness aggregation?
8. Why use max for pathVelocity aggregation?
9. How do you switch between Phase 1 and Phase 2 in one codebase?
10. Name the two functions where route-quality aggregation is done per hop.

### Round 1 Answer Key

1. Standard AODV ignores energy depletion and mobility instability in route quality.
2. Phase 1: energy + hop term. Phase 2: adds velocity penalty term.
3. So route metric can travel through request and reply paths end-to-end.
4. Needed for EnergySource and residual energy access inside routing protocol.
5. alpha=0.5, beta=0.3, gamma=0.2.
6. speed = sqrt(vx*vx + vy*vy + vz*vz).
7. Weakest-link policy for end-to-end path quality.
8. Worst-case mobility indicator along path.
9. By AODV attributes, especially Gamma and weight values.
10. RecvRequest and RecvReply.

## Round 2: Code-Level Depth (15 Questions)

1. Which exact member variables were added to RoutingProtocol for FF-AODV?
2. What guards exist in CalculateFitness to avoid invalid math?
3. Why is velocity clamped to maxVelocity before normalization?
4. In RecvRequest, at what point is hop count incremented relative to fitness computation?
5. What is the duplicate RREQ acceptance condition in your modified logic?
6. How is reverse route fitness stored the first time origin route is created?
7. Under what condition is an existing reverse route updated?
8. How is SendReply seeded with FF-AODV values when destination sends RREP?
9. What fallback does SendReply use if fitness or velocity is zero?
10. In RecvReply, how is fitness combined and then written back to header?
11. What changed in same-sequence-number route-update criterion in RecvReply?
12. Why is route-table fitness needed even after packet carries pathFitness?
13. What are new serialized sizes for RREQ and RREP after added doubles?
14. Why are doubles serialized via memcpy to uint64_t and network-byte-order writes?
15. Which simulation file toggles protocolVersion and maps it to Phase 1 vs Phase 2 weights?

### Round 2 Answer Key

1. m_alpha, m_beta, m_initialEnergy, m_gamma, m_maxVelocity.
2. Checks for initialEnergy > 0, hopCount > 0, maxVelocity > 0.
3. Prevents negative or invalid velocity contribution when speed exceeds normalization bound.
4. Hop is incremented first, then local fitness is computed using incremented hop.
5. Duplicate kept only if pathFitness improves over existing route-to-origin fitness.
6. newEntry.SetFitness(pathFitness) before AddRoute.
7. pathFitness > toOrigin.GetFitness().
8. replyFitness = rreqHeader.GetPathFitness(); replyVelocity = rreqHeader.GetVelocity().
9. Recompute from local residual energy and local speed.
10. replyFitness = min(incomingFitness, localFitness), then rrepHeader.SetPathFitness(replyFitness).
11. Compare replyFitness > storedFitness instead of lower-hop preference in same-seq case.
12. For future comparisons when new duplicates or replies arrive.
13. RREQ 39 bytes, RREP 35 bytes.
14. Bit-exact transport-safe serialization for double values.
15. scratch/experiment-ff-aodv.cc.

## Round 3: Blackboard Defense (10 Prompts)

For each prompt, explain structure first, then write compact code.

1. Write CalculateFitness for Phase 2 with all safety guards.
2. Show RREQ local metric extraction and path aggregation snippet.
3. Show duplicate-RREQ-improvement gate.
4. Show reverse route creation that stores fitness.
5. Show reverse route update condition based on improved fitness.
6. Show SendReply seeding from RREQ and fallback handling.
7. Show RecvReply path fitness update with min rule.
8. Show RecvReply path velocity update with max rule.
9. Show same-seq fitness-based route update condition.
10. Show one serialize and deserialize block for a double metric field.

### Round 3 Evaluation Rubric

A good answer must include:
- correct variable names and data flow
- correct order of operations
- clear handling of default/zero cases
- clear route-table update point
- no mismatch between packet state and route state

## Trap Questions Teachers Often Use

1. If Gamma is zero but velocity fields still exist in packets, is behavior wrong?
Expected: no, because velocity term weight is zero, so Phase 1 behavior is preserved.

2. Why not average fitness along path instead of min?
Expected: average can hide weak nodes; min is conservative and stability-focused.

3. Can duplicate acceptance create loops?
Expected: existing AODV safety checks and seq logic remain; only non-improving duplicates are blocked early.

4. Does adding 16 bytes per control packet matter?
Expected: overhead increases but acceptable for research tradeoff and explicit path-quality signaling.

5. What if no MobilityModel exists on node?
Expected: currentSpeed defaults to 0.0, so velocity contribution becomes best-case for that node unless overridden by incoming max.

## Timed Practice Plan

Day 1:
- Round 1 only, 10 minutes
- review weak points

Day 2:
- Round 2, 20 minutes
- rewrite wrong answers by hand

Day 3:
- Round 3, 30 minutes
- no notes, only blank page

Day 4 (final prep):
- full 70-point simulation under time
