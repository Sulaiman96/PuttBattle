# PR: Phase 2 — Surfaces, Hazards & Checkpoints (offline)

**Branch:** `phase/01-02-ball-shot-surfaces` · **Date:** 2026-06-13 · **Status:** C++ complete &
compiling. Surface asset set + UA-10 feel sign-off remain.

## What this delivers
The data-driven surface framework (with the global override stack the Ice Age powerup will reuse with
zero changes), the boost actor, hazards, checkpoints, and the single respawn path — all from
`docs/plans/02-surfaces-hazards-checkpoints.md`.

### T2.1 — Surface framework + override stack
- `Surfaces/PBSurfaceDefinition` (`UPrimaryDataAsset`): `SurfaceTag`, `RollDragMultiplier`,
  `FrictionOverride`, `RestitutionOverride`, `EPBSurfaceHook` (enum, not a tag — D2), optional Niagara
  `RollFX`.
- `Surfaces/PBPhysicalMaterial` (`UPhysicalMaterial`): holds the definition and mirrors its
  friction/restitution onto its own engine fields on load/edit, so a trace hit resolves both the Chaos
  numbers and the gameplay data in one lookup.
- `Surfaces/PBSurfaceSamplerComponent` (on the pawn): down sphere-trace on `PB_Floor` each tick →
  active definition → a **custom rolling-drag acceleration** opposing horizontal velocity, scaled by
  `RollDragMultiplier`, mass-independent (`AddForce` `bAccelChange`), only while grounded (airborne ramp
  shots untouched, D3). `pb.Surface.DragCoefficient` tunes it live.
- `Surfaces/PBSurfaceSubsystem` (WorldSubsystem): `PushGlobalOverride(Def, Duration)` / `PopGlobalOverride`
  / `RemoveOverride`, top-of-stack wins, auto-expiry via timers. The sampler consults
  `ResolveActiveDefinition()` so a pushed override outranks the contact surface everywhere.

### T2.2 — Boost actor (the one surface needing code)
- `Surfaces/PBBoostVolume` — box trigger + direction arrow; accelerates overlapping balls along the
  arrow forward while inside. Generic enough to be placed content. The five surface **assets**
  (Fairway/Ice/Sand/Sticky/Boost) are content (zero new code) — the editor pass.

### T2.3 — Hazards + checkpoints + respawn
- `Course/PBCheckpointActor` — sphere trigger, ordered `CheckpointIndex`, per-ball activation tracked
  server-side in the GameMode (keyed per ball now, per `PlayerState` in Phase 3), local-only activation
  FX hook.
- `Course/PBHazardVolume` — `Hazard.Water`/`Hazard.Void` box trigger.
- `APBGameMode::RespawnAtCheckpoint` — the single reset path: teleport to the highest activated
  checkpoint (else tee), zero velocity, no stroke change (D7). `APBBallPawn::FellOutOfWorld` routes
  KillZ falls into the same path instead of destroying the ball.

### Tests
- `Tests/PBSurfaceRollTest.cpp` — `PuttBattle.Tests.Surfaces.RollDistanceOrdering` integrates the shared
  `UPBSurfaceSamplerComponent::ComputeRollDeceleration` model and asserts roll-distance ordering
  Ice > Fairway > Sand > Sticky (and Ice ≥ 4× Sticky), logging the distances. Deterministic, no physics
  scene — so a feel change to the model is regression-caught.

## Verification evidence
- Builds clean in both the **game** and **editor** targets (zero errors/warnings;
  `UnrealEditor-PuttBattle.dll` linked).
- **Automation test PASSES** (in-editor, headless `UnrealEditor-Cmd … -ExecCmds="Automation RunTests
  PuttBattle.Tests.Surfaces.RollDistanceOrdering"`): `Result={Success}`, exit code 0, with
  `[RollDistance] Ice=5303.4 Fairway=1313.4 Sand=363.4 Sticky=205.0 (cm)` — ordering and the ≥4×
  Ice/Sticky distinctness both hold. (It can't run via the uncooked game exe — that crashes booting the
  uncooked asset registry, an editor-vs-game content issue, not a code one.)

## Requires editor (after the game module is rebuilt + the editor restarted)
1. **Surface assets:** `UPBPhysicalMaterial` + `UPBSurfaceDefinition` pairs for Fairway (baseline), Ice
   (drag ×0.25, restitution up), Sand (×3.5), Sticky (×6, restitution 0), Boost; tags from §5. Then the
   **Mud** "zero-code" proof (one more pair).
2. **Test strips** in the graybox map to log real roll distances; assign the physical materials to floors.
3. **Hazards/checkpoints** placed + a WorldSettings KillZ; splash VFX in BP.
4. **Run** `PuttBattle.Tests.Surfaces.RollDistanceOrdering` in the editor and confirm green.
5. **UA-10 feel sign-off** — gates phase exit.
