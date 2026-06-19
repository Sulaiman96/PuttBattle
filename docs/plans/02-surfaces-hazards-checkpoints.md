# Phase 2 — Surfaces, Hazards & Checkpoints (offline)

**Context:** Phases 0–1 complete. The surface framework built here must already support **global overrides** — the Ice Age powerup (Phase 6) pushes a whole-map surface swap and must require zero changes here.

> **Status 2026-06-13 (branch `phase/01-02-ball-shot-surfaces`):** ✅ **C++ + CONTENT PASS COMPLETE**,
> verified in PIE. Framework / override stack / sampler / boost / hazards / checkpoints / respawn in C++;
> roll-distance ordering covered by `PuttBattle.Tests.Surfaces.RollDistanceOrdering`. The **surface asset
> set** (6 `UPBSurfaceDefinition` + 6 `UPBPhysicalMaterial`, incl. the **Mud zero-code proof**) and the
> map integration (surface bands, boost volume, water hazard, checkpoints, KillZ) are done and committed.
> PIE confirms the sampler resolves `Surface.Fairway`/`Surface.Ice` via the `PB_Floor` object-type trace.
> **Only UA-10 feel sign-off remains.** Details in `docs/pr/phase-02-surfaces-hazards-checkpoints.md`.

## T2.1 — Surface framework + override stack  ✅ done (C++ + content 2026-06-13)
> `Surfaces/PBSurfaceDefinition` (PrimaryDataAsset: SurfaceTag, RollDragMultiplier, Friction/Restitution
> overrides, `EPBSurfaceHook`, optional Niagara RollFX), `Surfaces/PBPhysicalMaterial` (UPhysicalMaterial
> holding the definition; mirrors friction/restitution onto its engine fields on load/edit),
> `Surfaces/PBSurfaceSamplerComponent` (down sphere-trace on PB_Floor each tick → active definition →
> custom rolling-drag accel opposing horizontal velocity, mass-independent, only while grounded),
> `Surfaces/PBSurfaceSubsystem` (override stack `PushGlobalOverride(Def,Duration)`/`Pop`/`Remove`, top
> wins, auto-expiry timers). `pb.Surface.DragCoefficient` for live tuning. The pure deceleration model
> is `UPBSurfaceSamplerComponent::ComputeRollDeceleration` — shared with the automation test.
> **Spec test PASSES in-editor** (`Result={Success}`): `[RollDistance] Ice=5303.4 Fairway=1313.4
> Sand=363.4 Sticky=205.0 (cm)` — ordering + ≥4× Ice/Sticky distinctness confirmed.
> **Editor pending:** the three test strips to log real *physics* roll distances in PIE.
**Goal:** Data-driven per-surface ball behaviour.
**Create:**
- `Surfaces/PBPhysicalMaterial` (subclass `UPhysicalMaterial`, holds `UPBSurfaceDefinition*`).
- `Surfaces/PBSurfaceDefinition` (PrimaryDataAsset): `SurfaceTag`, `RollDragMultiplier`, `FrictionOverride`, `RestitutionOverride`, `GameplayHookTag` (none/boost/sticky/hazard), optional FX refs.
- `Surfaces/PBSurfaceSamplerComponent` on the pawn: short sphere-trace down each tick while moving; resolve contact `UPBPhysicalMaterial` → active definition. Apply a **custom rolling-drag force** (opposing horizontal velocity, ∝ speed × RollDragMultiplier) — Chaos friction alone won't give controllable feel; document chosen constants in the asset.
- `Surfaces/PBSurfaceSubsystem` (WorldSubsystem): **override stack** — `PushGlobalOverride(Def, Duration)` / `PopGlobalOverride()`. Sampler consults the top of the stack before contact material.
**Done when:** three test strips (Fairway/Ice/Sand) produce measurably different roll distances from identical impulses (log distances in an Automation test); pushing a global Ice override flips behaviour everywhere and auto-expires.

## T2.2 — Surface set ∥  ✅ done (boost actor + 6 surface asset pairs incl. Mud zero-code proof, 2026-06-13)
> `Surfaces/PBBoostVolume` (box trigger + arrow; accelerates overlapping balls along the arrow while
> inside) is the one boost-needs-an-actor piece, implemented. The Fairway/Ice/Sand/Sticky/Boost asset
> pairs are **content** (PhysicalMaterial + SurfaceDefinition + tag, zero code) and are the editor pass.
> **Editor pending:** author the five definitions + physical materials; the Mud "zero-code" proof
> follows once they exist.
**Create as assets (target: zero new C++):** Fairway (baseline), Ice (drag ×0.25, restitution up), Sand (drag ×3.5), Sticky (drag ×6, restitution 0), Boost — Boost needs one small actor: `PBBoostVolume` (direction arrow + force while overlapped), generic enough that boost pads are placed content.
**Done when:** a sixth surface, **Mud**, is added by a follow-up agent instruction using assets only — if that requires code, T2.1 failed its design.

## T2.3 — Hazards + checkpoints  ✅ done (C++ + content 2026-06-13)
> `Course/PBCheckpointActor` (sphere trigger, ordered `CheckpointIndex`, per-ball activation tracked
> server-side in the GameMode, local-only activation FX hook), `Course/PBHazardVolume`
> (`Hazard.Water`/`Hazard.Void` box trigger). `APBGameMode::RespawnAtCheckpoint` is the single reset
> path — teleport to the highest activated checkpoint (else tee), zero velocity, no stroke change (D7);
> `APBBallPawn::FellOutOfWorld` routes KillZ to that same path instead of destroying the ball.
> **Editor pending:** place checkpoints/hazards + a KillZ in the test map; splash VFX in BP.
**Create:**
- `Course/PBCheckpointActor`: sphere trigger; per-player activation (design for per-PlayerState tracking now, single-player map of one); ordered index; subtle activation FX visible only to the activating player (local-only in P1, replicated filtering in P3).
- `Course/PBHazardVolume` (`Hazard.Water` splash variant, `Hazard.Void`), plus WorldSettings KillZ handling routed to the same path: `RespawnAtCheckpoint(Player)` — teleport to latest activated checkpoint (else TeePad), zero velocity, no stroke change (D7).
**Done when:** ball into water → splash → respawn at last touched checkpoint; ball off a ramp past KillZ → same; with no checkpoints touched → tee respawn.

## Phase exit
Surface distinctness is verified objectively by the roll-distance test; **readable feel is human** — UA-10 sign-off gates the phase.
