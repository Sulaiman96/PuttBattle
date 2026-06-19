# Phase 1 — Ball & Shot (offline)

**Context:** Phase 0 complete. Everything here runs single-player; replication comes in Phase 3, but structs and authority boundaries are designed for it now.

> **Status 2026-06-13 (branch `phase/01-02-ball-shot-surfaces`):** ✅ **C++ + CONTENT PASS COMPLETE**,
> verified in PIE. T1.1–T1.4 implemented in C++; the editor content pass (BP subclasses `BP_BallPawn` /
> `BP_PBPlayerController` / `BP_PBGameMode`, input assets `IMC_Putt` + `IA_*`, graybox map
> `Maps/Holes/H_Test/V_A`, `WBP_HUD`) is done and committed to `Content/`. PIE confirms ball spawn/rest,
> HUD on viewport, surface resolution. **Only UA-9 feel sign-off remains** (gates phase exit). Details +
> evidence in `docs/pr/phase-01-ball-and-shot.md`.

## T1.1 — APBBallPawn + attributes  ✅ done (C++ + content 2026-06-13)
> `Ball/PBBallPawn.{h,cpp}` + `Ball/PBBallAttributes.h`. Sphere root (PB_BallProfile, CCD, sim
> physics), cosmetic mesh child (NoCollision), replicated `FPBBallAttributes` applied via
> `ApplyAttributes()` (scale→SetWorldScale3D + density-preserving mass). Feel constants are
> `EditAnywhere/BlueprintReadWrite` UPROPERTYs mirrored by `pb.Ball.{AngularDamping,LinearDamping,
> MaxImpulse,MaxSpeed}` CVars (CVar≥0 overrides) + `pb.Ball.Dump`/`SetScale`/`Invert`/`Lock`/`HideAim`
> console commands. **Editor pending:** `BP_BallPawn` wiring a sphere mesh + power curve; set as
> DefaultPawn. Console scale-to-1.6 mass check is UA-9-adjacent.
**Goal:** A physically believable rolling ball pawn with the attributes pattern installed.
**Create:** `Ball/PBBallPawn.{h,cpp}`
- Root: `USphereComponent` (radius ~6 cm scaled to your world metric — pick 1uu=1cm), `SimulatePhysics`, profile `PB_BallProfile`, CCD **on** (fast balls vs thin walls), angular damping tuned (start 0.6), linear damping 0.05.
- Child `UStaticMeshComponent` cosmetic mesh, `NoCollision`.
- `FPBBallAttributes` struct exactly per CONVENTIONS §3, replicated, applied only via `ApplyAttributes()` (scale → SetWorldScale3D + mass update; other flags read by ShotComponent/CupActor).
- Expose every feel constant (mass, dampings, max speed clamp) as `EditAnywhere, BlueprintReadWrite` UPROPERTYs — BP subclass `BP_BallPawn` wires mesh + a `UCurveFloat` power curve asset and carries the tuned values (D21: feel iteration never requires a recompile).
- Register mirrored console variables for live in-PIE tuning: `pb.Ball.AngularDamping`, `pb.Ball.LinearDamping`, `pb.Ball.MaxImpulse` etc. (FAutoConsoleVariableRef pattern; CVar overrides the BP value when set, logs current values via `pb.Ball.Dump`).
**Done when:** ball rolls, slows, and rests believably on a flat graybox plane; pushing scale to 1.6 via console test updates physics mass and visuals.

## T1.2 — UPBShotComponent (flat shots)  ✅ done (C++ + content 2026-06-13)
> `Shot/PBShotComponent.{h,cpp}` + `Shot/PBShotTypes.h` (`FPBShotRequest`, `EPBShotState`). State
> machine Idle→Aiming→Rolling with at-rest (|v|<5cm/s for 0.5s) + 12s roll-timeout. `APBPlayerController`
> (`Match/`) captures the LMB drag and streams it in; aim is camera-relative + flat (rotate screen
> drag by camera yaw, zero Z, negate unless `bAimInverted`); power = clamped drag / 0.28·viewportH
> through `PowerCurve`. Single funnel `ExecuteShot(FPBShotRequest)` (future server RPC) applies a flat
> mass-dependent impulse × `PowerMultiplier`; refuses when `bShotLocked`; preview hidden when
> `bAimIndicatorHidden`. Feel via `pb.Shot.{MaxDragFraction,DeadZone,AtRestSpeed,RollTimeout}`.
> **Editor pending:** `IMC_Putt` + `IA_DragShot/CycleCamera/CancelTargeting`, `BP_PBPlayerController`
> wiring them, Niagara/spline preview in BP (debug line works now via `pb.Shot.DrawDebugPreview`).
**Goal:** The drag-aim-release loop, feeling right on mouse and trackpad.
**Create:** `Shot/PBShotComponent.{h,cpp}` on the pawn; input handled by `APBPlayerController`.
- States: `Idle → Aiming → Rolling → Idle`. Aiming allowed only when at-rest: `|v| < 5 cm/s` sustained 0.5 s, or 12 s roll timeout force-stops (zero velocity).
- Drag: screen-space vector from press point to current; clamp length at `0.28 × viewport height`; power01 = clamped length / max, through the power curve. Cancel: release inside a 24 px dead-zone.
- **Aim is camera-relative and flat:** project drag direction onto the ground plane through the current camera, negate (ball goes opposite the drag), zero the Z. Final impulse = `Dir2D × power01 × MaxImpulse × Attributes.PowerMultiplier` (respect `bAimInverted` by not negating; respect `bShotLocked` by refusing).
- Preview: while aiming, dotted line (Niagara or spline mesh) from ball along aim dir to first blocking raycast hit, length also encoding power; hidden if `bAimIndicatorHidden`.
- Route the final impulse through one function `ExecuteShot(FPBShotRequest)` — in Phase 3 this becomes the body of the server RPC. `FPBShotRequest { FVector2D Dir; float Power01; }` in `Shot/PBShotTypes.h`.
**Done when:** a test hole is completable mouse-only; full power reachable with a 0.28-viewport drag; trackpad sanity-check passes; inverted/locked/hidden attribute flags all observably work via console toggles. Shot-feel constants (rest thresholds, drag fraction, max impulse) follow the same UPROPERTY + `pb.Shot.*` CVar pattern as T1.1 so UA-9 can be tuned live in one session.

## T1.3 — Graybox hole, cup, HUD  ✅ done (C++ + content 2026-06-13)
> `Course/PBCupActor` (sphere catch zone; sinks iff overlap AND speed < SinkSpeedThreshold AND
> ScaleMultiplier < 1.3 per DF6, re-checked each tick so a fast-then-settled ball still drops),
> `Course/PBTeePad` (an `APlayerStart` with `GetSpawnTransform()`), `Match/PBGameMode` (DefaultPawn +
> PC classes, `RestartHole`), `UI/PBHUDWidget` (C++ base that auto-binds stroke/aim/sunk delegates and
> forwards to BP visual events). **Editor pending:** graybox map `Maps/Holes/H_Test/V_A` with ramps
> (verify D3 off-map launch), `WBP_HUD` visuals (stroke counter, power bar, "SUNK!" toast), restart key.
**Goal:** A playable hole loop.
**Create:** `Course/PBCupActor` (cylinder trigger: sink iff overlap AND speed < SinkSpeedThreshold AND `ScaleMultiplier < 1.3` per DF6), `Course/PBTeePad` (spawn transform), graybox map `Maps/Holes/H_Test/V_A` with ramps (verify D3: fast ramp shots go airborne and can leave the map), WBP_HUD: stroke counter, power bar during aim, "SUNK!" toast, restart key.
**Done when:** spawn → shots counted → sink detected → restart works; a max-power ramp shot flies off the map edge.

## T1.4 — Camera rigs ∥  ✅ C++ done; ⚠️ player camera redesigned per **D-7** (2026-06-13)
> The C++ fixed-rig system (`PBCameraRig` + `PBCameraSubsystem`, cycle with C) is implemented, BUT the
> human redirected the **player** camera to 3 third-person follow presets (zoomed-in / mid / top-down)
> switched with **1/2/3** — a spring-arm follow camera on `BP_BallPawn`, not the fixed rigs (DECISIONS
> **D-7**). The graybox rigs were removed; the C++ rig classes remain (unused). README D4, the hole-intro
> glance (D20), and the spectator rig-cycling (Phase 8) still reference fixed rigs and need reconciling
> when those phases land.
> `Camera/PBCameraRig` (CameraComponent actor, `Priority`, default ~52° pitch / FOV 35) +
> `Camera/PBCameraSubsystem` (WorldSubsystem: collects rigs sorted by priority, `CycleCamera()` blends
> ViewTarget 0.3s, client-only). The controller binds `IA_CycleCamera` → `CycleCamera`. Aim stays
> correct after a cycle because the shot component re-derives camera-relative aim every frame (D2).
> **Editor pending:** place 3 rigs in the test hole; confirm C-key cycling + aim correctness in PIE.
**Goal:** Per-hole fixed isometric cameras with cycling.
**Create:** `Camera/PBCameraRig` (CameraComponent on a positioned actor, `Priority` int, isometric framing ~50–55° pitch, FOV ~35 for compressed perspective), `Camera/PBCameraSubsystem` (collect rigs in loaded hole sorted by priority; `CycleCamera()` blends ViewTarget 0.3 s). Client-only; no replication.
**Done when:** test hole has 3 rigs; C key cycles smoothly; aiming stays correct after every cycle (camera-relative aim re-derives per D2/T1.2).

## Phase exit
Agent acceptance above covers the objective parts (clamp math, attribute flags, completable hole). **Feel is human:** phase exits only on UA-9 sign-off (USER-ACTIONS.md); fold the feedback into a tuning pass before Phase 2.
