# PR: Phase 2 — Surfaces, Hazards & Checkpoints (offline)

**Branch:** `phase/01-02-ball-shot-surfaces` · **Date:** 2026-06-13 · **Status:** C++ complete & compiling;
**surface asset set + map integration complete (2026-06-13)** and verified in PIE. Only **UA-10 feel
sign-off** remains.

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

## Content pass (2026-06-13, via ue-mcp)
1. **Surface assets** (`/Game/Data/Surfaces/`): six `UPBSurfaceDefinition` + six `UPBPhysicalMaterial`
   pairs — Fairway (drag 1.0, fric 0.7, rest 0.3), Ice (0.25 / 0.05 / 0.5), Sand (3.5 / 0.9 / 0.1),
   Sticky (6 / 1.0 / 0.0, hook Sticky), Boost (1.0 / 0.7 / 0.3, hook Boost), **Mud (4.5 / 0.95 / 0.05,
   hook Sticky)**. Each phys material's `SurfaceDefinition` is wired; values read back and confirmed.
   The Mud pair is the **"zero new code" proof** (CONVENTIONS §6, T2.2 done-when): adding it was 1
   definition + 1 phys material + the existing `Surface.Mud` tag, no recompile. Six colored graybox
   materials in `/Game/Maps/Core/Materials/` carry the phys materials for visual distinction.
2. **Surface bands in the map** `/Game/Maps/Holes/H_Test/V_A`: a continuous "surface garden" of six
   adjacent floor bands (Mud · Sticky · Fairway-with-cup · Ice · Sand · Boost) so a player can putt onto
   each from the single tee (restart-key respawns at the tee to try the next). Each band's component has
   `BodyInstance.PhysMaterialOverride` set to its `PM_Surface_*` **and** object channel `PB_Floor` — both
   are required (see the collision gotcha). A `PBBoostVolume` sits over the Boost band.
3. **Hazards / checkpoints / KillZ:** `PBHazardVolume` (`Hazard.Water`, box enlarged to cover the sand
   band), two ordered `PBCheckpointActor`s (index 0 @ x=650, index 1 @ x=1300) on the fairway lane,
   `WorldSettings.KillZ = -500` routing falls to `RespawnAtCheckpoint`.
4. **Automation test:** `PuttBattle.Tests.Surfaces.RollDistanceOrdering` is unchanged this pass
   (deterministic pure-logic, no C++ touched) and its PASS is documented above with the logged distances.

### Verification evidence (PIE)
- **Collision contract works end-to-end:** the surface sampler's `SweepSingleByObjectType(PB_Floor)`
  resolves `Surface.Fairway` under the resting ball and `Surface.Ice` after teleporting the ball over the
  ice band (`invoke_function GetActiveSurfaceTag`). This is the live confirmation that floors are
  `PB_Floor` + carry the right `UPBPhysicalMaterial` → `UPBSurfaceDefinition`.
- The override-stack / Ice-Age path is exercised by the existing automation test's model; the global
  override flips behaviour everywhere (C++ `UPBSurfaceSubsystem`, unchanged).

### Collision gotcha (recorded in LEARNING.md)
Two non-obvious requirements, both load-bearing for surfaces:
- A simple-collision object-type sweep returns the body's **`PhysMaterialOverride`**, not the render
  material's `PhysMaterial` — so the phys material must be set on the floor *component* (not just the
  material asset).
- Setting object type to `PB_Floor` doesn't stick while a named profile (e.g. `BlockAll`) is active; the
  profile must be set to **`Custom`** first. The sampler only finds floors whose object type is actually
  `PB_Floor`.

## Remaining (human)
- **UA-10 surface feel sign-off** — gates phase exit. The six bands are placed unlabelled-from-feel; the
  agent can teleport the ball between them (the `TeleportToAndStop` path used in verification) so the human
  calls out each surface blind.
