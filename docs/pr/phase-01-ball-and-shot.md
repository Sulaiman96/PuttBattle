# PR: Phase 1 — Ball & Shot (offline)

**Branch:** `phase/01-02-ball-shot-surfaces` · **Date:** 2026-06-13 · **Status:** C++ complete &
compiling (game target links clean). Editor content pass + UA-9 feel sign-off remain.

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

## Requires editor (after the game module is rebuilt + the editor restarted)
These need the new classes registered in the editor, so they're the next pass (MCP-drivable):
1. **Input assets:** `IMC_Putt` + `IA_DragShot` (LMB), `IA_CycleCamera` (C), `IA_CancelTargeting`
   (RMB/Esc) — also clears the Phase 0 deferral.
2. **BP subclasses:** `BP_BallPawn` (sphere mesh + `UCurveFloat` power curve + tuned feel),
   `BP_PBPlayerController` (wire the input assets), `BP_PBGameMode` (use them); set DefaultPawn/PC.
3. **Graybox map** `Maps/Holes/H_Test/V_A` with ramps (verify D3: a max-power ramp shot leaves the map),
   a tee, a cup, and 3 camera rigs; floors/walls on `PB_Floor`/`PB_Wall` (CONVENTIONS §4).
4. **`WBP_HUD`** (subclass `UPBHUDWidget`): stroke counter, power bar, "SUNK!" toast, restart key.
5. **UA-9 feel sign-off** — gates phase exit (`docs/USER-ACTIONS.md`).
