# Phase 7 (ahead) — Hole 4 "The Slushie Chute" graybox · `V_A`

Second graybox hole (after H_03), built ahead of the Phase-7 variant system. It's the **ice
corridor / dogleg** that plans/07 T7.3 wants. Source: `Sugar Rush — 5 Holes Top-Down` doc, Hole 4
("4A Cold Run", PAR 3) — "a blue-raspberry chute where the dogleg forces a bank off the freezer rail."

Map: **`/Game/Maps/Holes/H_04_SlushieChute/V_A`** (`Content/Maps/Holes/H_04_SlushieChute/V_A.umap`, new/untracked).

## Build method (this is the important bit — fully functional, no hand-off)
The active editor MCP (native HTTP toolset server) **cannot set a component's collision channel**
(`PB_Floor`/`PB_Wall`) — only the old `execute_python` ue-mcp or the editor can (see
`docs/LEARNING.md`). Building floors/walls from scratch via the native MCP would leave them all on
`WorldStatic`, which would break the surface sampler (no ice!), Ghost Ball, and the §4 contract.

Workaround used here: **duplicated H_03 as the H_04 template, then re-shaped its already-channeled
meshes** (changed `staticMesh`, transform, `physMaterialOverride`, and render material via
`set_properties`). The PB_Floor/PB_Wall **object channels carry over untouched** because I never
write the collision field — and `physMaterialOverride` *is* writable, so Fairway floors become Ice.
Result: a genuinely functional ice hole (the corridor is real PB_Floor + `PM_Surface_Ice`), built
entirely in-session, no editor/old-mcp follow-up for collision. The trade-off is the build is
budgeted to H_03's channeled-mesh inventory: **3 PB_Floor + 4 PB_Wall** (see simplifications below).

## Layout (px→cm at 1 px = 10 cm; cup at world origin; north/up = +X, east/right = +Y)
Upside-down-L dogleg: a long horizontal ice arm, banking at the NE elbow down a vertical ice arm to
the cup. (Reused H_03 actor in brackets.)

| Element | Actor / mesh | Center (cm) | Scale | Channel · surface |
|---|---|---|---|---|
| Horizontal ice arm | `Ice_Horizontal` Cube [_9] | (2120, −1920, −10) | (14, 52, 0.2) → 1400×5200×20 | PB_Floor · Ice |
| Vertical ice arm (cup arm) | `Ice_Vertical` Cube [_10] | (1220, −20, −10) | (32, 14, 0.2) → 3200×1400×20 | PB_Floor · Ice |
| Mint brake-pad (before cup) | `Mint_BrakePad` Cube [_11] | (440, −20, −9) → top +1 | (4.4, 10.4, 0.2) | PB_Floor · Fairway |
| North rail | `Wall_Top` Cube [_12] | (2890, −1920, 60) | (1.4, 54.8, 1.2) → 120 tall | PB_Wall |
| East rail (the elbow bank) | `Wall_Right_Bank` Cube [_13] | (1220, 750, 60) | (34.8, 1.4, 1.2) | PB_Wall |
| Back wall (behind cup) | `Wall_Back` Cube [_14] | (−450, 50, 60) | (1.4, 15.4, 1.2) | PB_Wall |
| Inner-corner block (forces dogleg) | `Wall_InnerBlock` Cube [_15] | (605, −2130, 60) | (16.9, 29.8, 1.2) | PB_Wall |
| Tee (faces +Y down the corridor) | `Tee` PBTeePad [_4] | (2120, −4140, 20), yaw 90 | — | — |
| Cup | `Cup` PBCupActor [_1] | (0, 0, 0) | — | — |
| Checkpoint at the elbow | `Checkpoint_Elbow` [_2], index 0 | (2120, 230, 30) | — | — |
| Flag beacon | `Lolly_Pole`/`Lolly_Head` [_16/_17] | (0, 0, …) rising over the rails | — | ⚠ WorldStatic (see below) |
| Cameras | `CameraRig_Frame` [_2] / `CameraRig_Overview` [_3] | Frame (−1500,−5500,3800) pitch −41 yaw 54; Overview auto-orbit at (1100,−1900), r=4000, h=3200 | — | — |
| Lighting / World | inherited from H_03 | — | KillZ −500, `BP_PBGameMode` | — |

**How the hole plays:** ball spawns at the tee (far west of the horizontal arm), is putted +Y (east)
along the ice to the NE elbow, **banks off the north/east rail** to slingshot south down the vertical
arm, crosses the **mint brake-pad** (normal friction, to kill ice speed) and drops in the cup. The
inner block forces the dogleg; the checkpoint sits at the elbow (no checkpoint past it — overshooting
the cup on ice is punished). Matches the design's Hero (one firm bank) / Safe (two soft taps, brake on
the mint pad) lines.

## Simplifications vs the design (greybox)
To fit H_03's channeled-mesh budget (3 floors + 4 walls) and keep the hole fully functional in-session:
1. **Dead-end alcove pocket omitted** — a decorative ice trap off the horizontal arm (held a goodie
   jar; pickups don't exist until Phase 5 anyway).
2. **Left (west/tee-end) freezer rail omitted** — the tee end is an open floor edge; a mishit backwards
   falls off → KillZ respawn. The four gameplay-critical walls (north, east-bank, back, inner block)
   are all present.

Add these later (1 more floor + 1 more wall) once channel-setting is available (old `execute_python`
ue-mcp or editor) — or by extending the reuse trick with more channeled source meshes.

## Known issues
- **Lolly beacon on `ECC_WorldStatic` (§4)** — same situation as H_03: the native MCP can't change its
  channel, so the decorative flag stays on a default channel. Minor gameplay impact (cup catch radius
  beats the thin pole). This is a **course-wide** decision: finish all hole beacons to `PB_Wall`
  (or `NoCollision`) via the old `execute_python` ue-mcp / editor in one pass.
- **Exposure / reflections** — the surfaces were rendering blown-out white and mirror-reflective
  (histogram auto-exposure over-adapting across a wide EV100 range). Fixed in this hole with an
  **unbound `PostProcessVolume` ("PostProcess_Global", priority 1)**: fixed exposure
  (`AutoExposureMethod` Histogram with `AutoExposureMin/MaxBrightness` both pinned to **EV100 5.0**,
  `ApplyPhysicalCameraExposure` off, bias 0), `BloomIntensity` 0.2, `ScreenSpaceReflectionIntensity` 0.
  EV100 5.0 was tuned by eye (15 → near-black, 8 → dusk, 5 → bright daylight without clipping); nudge
  `AutoExposureMin/MaxBrightness` together to taste (lower = brighter). H_03 still has the original
  blown-out look — apply the same volume there (or make it a shared course-wide setup).

## Verification
- Full MCP property inventory: 3 floors confirmed PB_Floor (`ECC_GameTraceChannel2`) — two with
  `PM_Surface_Ice`, the brake-pad with `PM_Surface_Fairway`; 4 walls PB_Wall (`ECC_GameTraceChannel3`);
  tee faces +Y; cup at origin; single checkpoint index 0 at the elbow; 24 actors total (the 3 spare
  tees + 1 spare checkpoint from the H_03 template were removed). Level saved (not dirty).
- Top-down + framing-camera captures confirm the L-dogleg geometry, walls standing on the floor, the
  inner-block notch, and that the framing camera frames the hole.
- Not yet playtested in PIE (ball-feel on ice + the elbow bank is a human/feel check).

## Gameplay tuning (2026-06-19, post-build feedback)
Playtest feedback: ball wouldn't sink, shots felt slow, walls didn't bounce, and aiming required a
full stop. Fixes (all no-recompile — feel values on the BP / instances / a new material):

- **Sinking** — the Lolly beacon stood in the cup and the **ball is 25 cm radius** (50 cm — authored
  on `BP_BallPawn`, not the 6 cm C++ default), so a flagstick at the cup unavoidably blocks it; and on
  ice the ball stayed above the 120 cm/s sink threshold. Fix: **removed the Lolly** from this cup, and
  on `PBCupActor` raised **`SinkSpeedThreshold` 120 → 220** and **`CatchRadius` 12 → 18** (H_04 instance only).
- **Shot power** — `BP_BallPawn` **`MaxImpulse` 90 → 120** (global, all holes).
- **Wall bounce** — ball and walls both had no physics material (restitution 0). Added
  **`/Game/Data/Surfaces/PM_Wall`** (a `UPBPhysicalMaterial` with its `SurfaceDefinition` nulled so the
  value sticks: `Restitution` 1.0, `Friction` 0.2) and set it as the `PhysMaterialOverride` on the 4
  walls. Project `RestitutionCombineMode` is `Average`, so wall(1.0)+ball(0) → ~0.5 effective bounce.
  (Other holes' walls would need the same override; the material now exists to reuse. For a livelier
  bounce, switch `RestitutionCombineMode` to `Max` in Project Settings → Physics — needs an editor restart.)
- **Shoot when nearly stopped** — `BP_BallPawn`'s ShotComponent **`AtRestSpeedThreshold` 5 → 25 cm/s**
  and **`AtRestDuration` 0.5 → 0.35 s** (global), so a slow creep counts as "at rest" instead of
  requiring a dead stop.

All changed defaults verified by read-back; `BP_BallPawn` recompiled + saved. Effective on next Play.
