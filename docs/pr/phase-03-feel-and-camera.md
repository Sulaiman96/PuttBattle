# Phase 03 — Feel & Camera fixes (iteration on the 8-item findings list)

Running summary of the gameplay-feel fixes from in-PIE testing. Companion to
`docs/pr/phase-01-02-testing.md` (the test script that surfaced these).

## Shipped & live now (no rebuild — just Stop → Play)

| # | Item | Where | How |
|---|------|-------|-----|
| 1 | Ball size kept at radius 25 | `Content/Ball/BP_BallPawn` | `BaseRadiusCm=25`; cosmetic `BallMesh` scale synced to 0.5 (=radius/50). |
| 2a | Hole catch radius scaled to ball | `V_A` → `Cup` | `CatchRadius 12 → 30`. |
| 2b | **Ball mass tamed** (was flying off) | `BP_BallPawn` `BaseMassKg 0.045 → 0.1`; new `pb.Ball.SetMass` console cmd in `PBBallPawn.cpp` | Shot is flat impulse with `bVelChange=false`, so launch speed = `MaxImpulse / mass`. At 0.045 kg a full shot launched at ~2000 cm/s (rocket); 0.1 kg ≈ 900 cm/s. Tune live: `pb.Ball.SetMass 0.12`. `pb.Ball.Dump` now prints base+body mass. |
| 4 | Flag visible from afar | `V_A` → `HoleFlagPole` + `HoleFlag` | Graybox pole/flag at the cup. |
| 5 | Stroke counter | `PBHUDWidget.cpp` | Owning-player fallback + per-frame push of `PS->GetStrokes()`. |
| 3a | "SCORED!" toast | `WBP_HUD` `SunkText` | Centered, large; fired via the same owner-fallback. |
| 8a | Cam 1 angled up | `BP_BallPawn` `CameraBoom` | Boom pitch `-45 → -30`. |

## Shipped C++ — needs an editor-target rebuild + restart to load

The **camera redesign** (items 6 & 8) is written and **compile-verified against the
Game target (0 warnings, Result: Succeeded)**. It adds new UPROPERTYs / a USTRUCT /
a new input action, which Live Coding cannot hot-load — the editor must rebuild and
restart before these classes appear.

What the code does:
- **3 cameras on keys 1 / 2 / 3** (`APBPlayerController`):
  - **Cam 1 "Play"** — ball-follow, pitch -30, arm 650, FOV 75, **RMB-drag orbits it**.
  - **Cam 2 "Shot"** — ball-follow, pitch -45, arm 1150, FOV 80, **auto-faces the hole**
    (boom yaw eases toward ball→cup each tick) so ball + cup share the frame. Orbit off.
  - **Cam 3 "Overview"** — a separate **auto-orbiting world rig** (`APBCameraRig` with
    `bAutoOrbit`), a slow 360° map sweep, player-independent + time-driven (≈shared in MP).
- **Orbit moved to RMB** (item 6). Sensitivity = `OrbitYawSpeed` UPROPERTY (0.8), live
  override `pb.Camera.OrbitSpeed`. Face-hole ease speed = `pb.Camera.FaceHoleSpeed` (3).
- Framing values are `FPBFollowCamPreset` entries on `APBPlayerController` (BP-tunable).

Files: `Source/PuttBattle/Match/PBPlayerController.{h,cpp}`,
`Source/PuttBattle/Camera/PBCameraRig.{h,cpp}`.

### Input assets already wired (done this session, C++-independent)
- `IA_OrbitCamera` created (`/Game/Input/`).
- `IMC_Putt`: `RightMouseButton → IA_OrbitCamera` added; old `RMB → IA_CancelTargeting`
  removed. **Escape still cancels aim.** (1/2/3 were already mapped to `IA_Camera1/2/3`.)

## Remaining editor steps — DONE (2026-06-19, post-rebuild)

1. ~~Rebuild the editor target.~~ ✅ Done by the user (editor restarted).
2. ✅ **`BP_PBPlayerController`** wired (`/Game/Match/`): `Camera1Action=IA_Camera1`,
   `Camera2Action=IA_Camera2`, `Camera3Action=IA_Camera3`, `OrbitCameraAction=IA_OrbitCamera`.
   Compiled + saved.
3. ✅ **Overview rig placed in `V_A`:** `OverviewCam` (`PBCameraRig`) at (1200, 200, 0),
   `bAutoOrbit=true`, OrbitRadius 2200 / Height 1600 / FOV 55 / 6°·s⁻¹. Map saved.
4. ⏳ **Verify in PIE (user):** keys 1/2/3 switch cams; RMB-drag orbits cam 1; cam 2 keeps the
   hole framed; cam 3 slowly circles the whole map. Re-run the variant validator (plans/07)
   since the map changed.

## Known issues / deferred
- **Item 3 (true recessed hole):** still needs real floor-opening geometry — flag + catch
  zone cover visibility/scoring for now.
- Initial framing at possession relies on the BP boom default (= cam 1); press 1 to force.
- Cam 3 sync in MP is time-based, not frame-locked (fine for a cosmetic overview).
