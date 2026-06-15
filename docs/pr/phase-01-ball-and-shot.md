# PR: Phase 1 — Ball & Shot (offline)

**Branch:** `phase/01-02-ball-shot-surfaces` · **Date:** 2026-06-13 · **Status:** C++ complete & compiling;
**editor content pass complete (2026-06-13)** and verified in PIE. Only **UA-9 feel sign-off** remains.

## What this delivers
The full offline ball-and-shot system from `docs/plans/01-ball-and-shot.md`, in C++. Structs and the
authority boundary (`ExecuteShot`, attribute mutation, server-only respawn) are shaped for Phase 3
replication now, but everything runs single-player.

### T1.1 — Ball pawn + attributes
- `Ball/PBBallAttributes.h` — the `FPBBallAttributes` struct exactly per CONVENTIONS §3 (the one
  channel for every gameplay modifier), replicated.
- `Ball/PBBallPawn.{h,cpp}` — `USphereComponent` root (profile `PB_BallProfile`, sim physics, CCD on,
  damping), cosmetic mesh child (NoCollision), `ApplyAttributes()` (scale → `SetWorldScale3D` +
  density-preserving mass so a console scale visibly changes physics — D4). Feel as
  `EditAnywhere/BlueprintReadWrite` UPROPERTYs mirrored by `pb.Ball.{AngularDamping,LinearDamping,
  MaxImpulse,MaxSpeed}` CVars (a CVar ≥ 0 overrides the authored value), plus
  `pb.Ball.{Dump,SetScale,Invert,Lock,HideAim}` console commands for testing attribute flags.

### T1.2 — Shot component (flat drag-aim-release)
- `Shot/PBShotTypes.h` — `FPBShotRequest { FVector2D Dir; float Power01; }` (future RPC payload) +
  `EPBShotState`.
- `Shot/PBShotComponent.{h,cpp}` — Idle→Aiming→Rolling, at-rest (|v| < 5 cm/s for 0.5 s) + 12 s
  roll-timeout. Camera-relative flat aim (rotate the screen drag by camera yaw, zero Z, negate unless
  `bAimInverted`); power = clamped drag / (0.28 · viewport height) through `PowerCurve`; dead-zone
  cancel. One funnel `ExecuteShot()` applies a flat, **mass-dependent** impulse × `PowerMultiplier`;
  refuses on `bShotLocked`; preview suppressed on `bAimIndicatorHidden`. Feel via `pb.Shot.*`.
- `Match/PBPlayerController.{h,cpp}` — captures the LMB drag (Enhanced Input) and streams cursor
  position to the shot component each frame; binds camera-cycle + cancel.

### T1.3 — Cup, tee, GameMode, HUD
- `Course/PBCupActor` — sphere catch zone; sinks iff overlap **and** speed < `SinkSpeedThreshold`
  **and** `ScaleMultiplier < 1.3` (DF6), re-evaluated each tick so a ball that enters fast and settles
  still drops. Broadcasts `OnBallSunk`.
- `Course/PBTeePad` — an `APlayerStart` subclass exposing `GetSpawnTransform()` (spawn + respawn truth).
- `Match/PBGameMode` — sets DefaultPawn/PC classes, `RestartHole()`.
- `UI/PBHUDWidget` — C++ base that auto-binds stroke/aim/sunk delegates and forwards to BP visual events.

### T1.4 — Cameras
- `Camera/PBCameraRig` (CameraComponent actor, `Priority`, ~52° / FOV 35) + `Camera/PBCameraSubsystem`
  (WorldSubsystem; collects rigs by priority, `CycleCamera()` blends ViewTarget 0.3 s, client-only).

## Verification evidence
- **Both targets build clean:** `Build.bat PuttBattle Win64 Development` → linked `PuttBattle.exe`; and
  `Build.bat PuttBattleEditor Win64 Development` (editor closed) → linked `UnrealEditor-PuttBattle.dll`.
  Zero errors/warnings; UHT processed all headers with `-WarningsAsErrors`.

## Known issues / notes
- `WITH_EDITOR`-only paths (`PostEditChangeProperty`) aren't exercised by the game-target build; they
  compile in the editor target (trivial bodies).
- The aim preview currently draws a debug line (`pb.Shot.DrawDebugPreview 1`) until the BP
  Niagara/spline preview is wired.
- Stroke count lives on the shot component for Phase 1; it moves to `PlayerState` in Phase 3/4.

## Content pass (2026-06-13, via ue-mcp)
All editor-side work is done and committed to `Content/`:
1. **Input assets** (`/Game/Input/`): `IMC_Putt` mapping context + `IA_DragShot` (LMB), `IA_CycleCamera`
   (C), `IA_CancelTargeting` (RMB + Esc), `IA_Restart` (R). All Boolean actions; mappings verified via
   `read_imc` (5 mappings, default press→Started / release→Completed matches the C++ bindings).
2. **BP subclasses:**
   - `BP_BallPawn` (`/Game/Ball/`): cosmetic `BallMesh` = engine Sphere at scale 0.12 (→ 6 cm radius to
     match the C++ collision sphere), white `M_Ball`, `SurfaceSampler.FallbackDefinition` = `DA_Surface_Fairway`.
     `ShotComponent.PowerCurve` left **null** = linear identity (playable); the curve *shape* is a UA-9
     feel artifact (see Known issues).
   - `BP_PBPlayerController` (`/Game/Match/`): the four input UPROPERTYs assigned; `BeginPlay` graph creates
     `WBP_HUD` (OwningPlayer = self) and adds it to the viewport; `IA_Restart` → GetGameMode → Cast
     `PBGameMode` → `RestartHole(self)`.
   - `BP_PBGameMode` (`/Game/Match/`): `DefaultPawnClass` = `BP_BallPawn`, `PlayerControllerClass` =
     `BP_PBPlayerController`. Set as the map's GameMode override.
3. **Graybox map** `/Game/Maps/Holes/H_Test/V_A`: a flat fairway lane (tee → cup) flanked by walls, an
   incline ramp on the open far end (D3 off-map launch), a back wall, directional + sky + atmosphere
   lighting, `KillZ = -500`. Floors/ramp on object channel `PB_Floor`, walls on `PB_Wall` (CONVENTIONS §4)
   — see the collision gotcha below.

   **Camera (revised — DECISIONS D-7):** the player camera is now **3 third-person follow presets** on a
   spring arm on `BP_BallPawn` (boom absolute-rotation so the rolling ball's spin doesn't tumble it),
   switched with **1/2/3**: P1 zoomed-in (pitch −28°, FOV 50), P2 mid/default (−45°, FOV 75), P3 top-down
   angle (−72°, FOV 88). This supersedes the original 3 fixed `PBCameraRig`s (D4) for the player — the
   rigs were removed from the map and `C` cycling retired; the C++ rig system remains unused. PIE-verified:
   the active view matches each preset's spring-arm math exactly.
4. **`WBP_HUD`** (`/Game/UI/`, parent `UPBHUDWidget`): `StrokeText` ("Strokes: {Count}" via Format Text on
   `OnStrokesChanged`), `PowerBar` (ProgressBar, anchored bottom-centre, shown/hidden + filled on
   `OnAimPower`, collapsed on `OnAimEnded`), `SunkText` ("SUNK!" centre, shown on `OnLocalBallSunk`).
   Compiles clean.

### Verification evidence (PIE)
- `BP_PBGameMode` spawns `BP_BallPawn` at the tee; ball **rests at z≈6** (= radius) on the fairway —
  collision + rest confirmed (`get_pie_pawn`, `GetPlanarSpeed` = 0).
- `WBP_HUD` is on the viewport with `StrokeText` = "Strokes: 0" (`widget list_runtime` / `get_runtime`).
- Surface sampler resolves `Surface.Fairway` under the resting ball and `Surface.Ice` when teleported over
  the ice band (collision contract working end-to-end — see phase-02 PR).
- Cup→HUD: the cup's `OnBallSunk` delegate has the HUD bound (confirmed), and the `OnLocalBallSunk →
  SetVisibility(SunkText, Visible)` graph is wired and compiles. Visual confirmation of the toast on a real
  sunk putt is part of UA-9 (the live drag-shot loop needs human input that MCP can't inject).

## Remaining (human)
- **UA-9 feel sign-off** — gates phase exit (`docs/USER-ACTIONS.md`). The `pb.Ball.*` / `pb.Shot.*` CVars are
  live for tuning; the **power-curve shape** and **camera framing/FOV + lighting exposure** are feel/visual
  judgments to settle during this pass (the graybox lighting is currently bright/un-tuned).
