# Phase 2 — Surfaces, Hazards & Checkpoints (offline)

**Context:** Phases 0–1 complete. The surface framework built here must already support **global overrides** — the Ice Age powerup (Phase 6) pushes a whole-map surface swap and must require zero changes here.

## T2.1 — Surface framework + override stack
**Goal:** Data-driven per-surface ball behaviour.
**Create:**
- `Surfaces/PBPhysicalMaterial` (subclass `UPhysicalMaterial`, holds `UPBSurfaceDefinition*`).
- `Surfaces/PBSurfaceDefinition` (PrimaryDataAsset): `SurfaceTag`, `RollDragMultiplier`, `FrictionOverride`, `RestitutionOverride`, `GameplayHookTag` (none/boost/sticky/hazard), optional FX refs.
- `Surfaces/PBSurfaceSamplerComponent` on the pawn: short sphere-trace down each tick while moving; resolve contact `UPBPhysicalMaterial` → active definition. Apply a **custom rolling-drag force** (opposing horizontal velocity, ∝ speed × RollDragMultiplier) — Chaos friction alone won't give controllable feel; document chosen constants in the asset.
- `Surfaces/PBSurfaceSubsystem` (WorldSubsystem): **override stack** — `PushGlobalOverride(Def, Duration)` / `PopGlobalOverride()`. Sampler consults the top of the stack before contact material.
**Done when:** three test strips (Fairway/Ice/Sand) produce measurably different roll distances from identical impulses (log distances in an Automation test); pushing a global Ice override flips behaviour everywhere and auto-expires.

## T2.2 — Surface set ∥
**Create as assets (target: zero new C++):** Fairway (baseline), Ice (drag ×0.25, restitution up), Sand (drag ×3.5), Sticky (drag ×6, restitution 0), Boost — Boost needs one small actor: `PBBoostVolume` (direction arrow + force while overlapped), generic enough that boost pads are placed content.
**Done when:** a sixth surface, **Mud**, is added by a follow-up agent instruction using assets only — if that requires code, T2.1 failed its design.

## T2.3 — Hazards + checkpoints
**Create:**
- `Course/PBCheckpointActor`: sphere trigger; per-player activation (design for per-PlayerState tracking now, single-player map of one); ordered index; subtle activation FX visible only to the activating player (local-only in P1, replicated filtering in P3).
- `Course/PBHazardVolume` (`Hazard.Water` splash variant, `Hazard.Void`), plus WorldSettings KillZ handling routed to the same path: `RespawnAtCheckpoint(Player)` — teleport to latest activated checkpoint (else TeePad), zero velocity, no stroke change (D7).
**Done when:** ball into water → splash → respawn at last touched checkpoint; ball off a ramp past KillZ → same; with no checkpoints touched → tee respawn.

## Phase exit
Surface distinctness is verified objectively by the roll-distance test; **readable feel is human** — UA-10 sign-off gates the phase.
