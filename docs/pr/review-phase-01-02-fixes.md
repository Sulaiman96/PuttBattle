# PR: Review & fixes — phase 1-2 ball/shot/surfaces

**Branch:** `phase/01-02-ball-shot-surfaces` · **Date:** 2026-06-15 · **Status:** C++ + config fixes applied;
**editor target builds clean (zero new warnings in `PuttBattle`)**; roll-distance automation test passes.

A focused review of the phase 1-2 implementation (ball pawn, shot loop, surfaces/override stack, hazards,
checkpoints, cup, camera, HUD) plus the high-confidence fixes that came out of it. No new features — these
are correctness, feel-tuning-surface (§1), and future-proofing repairs.

## Correctness fixes
- **Planar speed clamp (D3)** — `APBBallPawn::Tick` clamped the full 3D velocity, so a fast roll up a ramp
  had its vertical launch proportionally throttled (the opposite of D3 "ramp shots can fly off the map").
  Now clamps the **planar** component only (`GetClampedToMaxSize2D` / `SizeSquared2D`), leaving Z intact.
  `Ball/PBBallPawn.cpp`.
- **Airborne drag during Ice Age** — the surface sampler braked an *airborne* ball whenever a global
  override was active (`bOverride` slipped past the grounded guard), contradicting D3 + the component's own
  doc. Roll-drag now applies only to a **grounded** ball regardless of override. `Surfaces/PBSurfaceSamplerComponent.cpp`.
- **Override stack GC-rooting** — `FOverrideEntry` was a plain struct, so its `TObjectPtr` definition was
  invisible to GC and could be collected mid-override (the exact Ice-Age case the stack exists for).
  Promoted to `USTRUCT FPBSurfaceOverride` with a `UPROPERTY()` definition + `UPROPERTY(Transient)` array
  (D-9). `Surfaces/PBSurfaceSubsystem.{h,cpp}`.
- **Cup sink authority** — `APBCupActor::Tick` ran sink truth (mutate + teleport + broadcast) with no
  `HasAuthority()` guard, unlike the sibling course actors. Added the guard (no-op in standalone; in Phase 3
  it stops every client independently sinking the replicated ball). `Course/PBCupActor.cpp`.
- **ExecuteShot input validation** — the shot funnel (the verbatim future `Server_ExecuteShot` body) now
  clamps `Power01` to `[0,1]` internally, not just upstream. `Shot/PBShotComponent.cpp`.
- **Aim-preview trace channel** — preview line now traces the `PB_Wall`/`PB_Floor` object channels instead
  of `ECC_Visibility` (which the §4 contract makes no promise about). `Shot/PBShotComponent.cpp`.

## Feel-tuning surface (§1: every feel value reachable without recompiling)
Three baked constants were exposed as `EditAnywhere/BlueprintReadWrite` UPROPERTYs:
- `APBBallPawn::BaseRadiusCm` (was `InitSphereRadius(6)`) — ball size is core feel.
- `UPBShotComponent::PreviewLengthAtFullPower` / `PreviewMinLength` (was `Max(50, Power*600)`).
- `APBCupActor::CatchRadius` (was `InitSphereRadius(12)`) — sits beside SinkSpeedThreshold/BalloonedMaxScale.
- `APBBoostVolume::ZoneExtent` + new `MaxBoostSpeed` cap (boost is now self-bounding, independent of the
  pawn clamp).

## Config / project integrity
- **Default maps** (`EditorStartupMap`/`GameDefaultMap`/`ServerDefaultMap`) pointed at the never-created
  `L_Bootstrap`; repointed to the real `V_A` map so the editor + any standalone launch land in a valid
  world (D-8). Revert once a real `L_Bootstrap` ships.
- **`GlobalDefaultGameMode`** set to `BP_PBGameMode_C` so the D7 respawn path works on every map, not only
  ones with a per-map override (D-8).
- Deleted the stray, unreferenced `Content/Untitled.umap`.

## Cleanups
- Removed the redundant constructor mass write (ApplyAttributes owns mass, §3); removed two empty
  `BeginPlay` overrides; cached the per-tick damping write so it only pokes Chaos on change; dropped an
  unnecessary `const_cast` in the player controller.
- **HUD binding** is now possession-aware (`OnPossessedPawnChanged`) so the stroke/power UI binds reliably
  even if the widget is created before the ball is possessed, and re-binds after respawn. `UI/PBHUDWidget.{h,cpp}`.

## Verification
- `Build.bat PuttBattleEditor Win64 Development` (editor closed) → **Result: Succeeded**, all 8 changed
  `PuttBattle` units recompiled, UHT `-WarningsAsErrors` passed, module linked. Zero warnings in `PuttBattle`
  (the only C4996 warnings are pre-existing, in the vendored `UE_MCP_Bridge` editor plugin).
- `PuttBattle.Tests.Surfaces.RollDistanceOrdering` → **Result={Success}**, `Ice=5303.4 Fairway=1313.4
  Sand=363.4 Sticky=205.0` (unchanged — roll model untouched).

## Deferred (documented, not changed) — see review for full list
- `UPBPhysicalMaterial` friction/restitution don't re-sync when the *definition* asset is edited at
  edit-time (needs a `WITH_EDITOR` push-back through Chaos `UpdateMaterial`). Editor-fidelity only.
- KillZ is per-map WorldSettings, undocumented for new maps — needs a map-authoring checklist item.
- Phase-3 watchlist: physics replication mode for the owned ball, StrokeCount → PlayerState, attribute
  net-serialization keeping ScaleMultiplier flushing, debug CVar authority routing.

## Remaining (human)
- **`Content/` is uncommitted** (`?? Content/`) despite earlier docs claiming it was committed — ~32 LFS
  assets (BP_BallPawn, surface DA_/PM_ pairs, input, WBP_HUD, gamemode/PC blueprints, the V_A map). Needs a
  review + commit (and a `git lfs status` check) before a clean clone would be playable. Not pushed.
- UA-9 (ball/shot feel) and UA-10 (surface feel) sign-offs still gate the phases.
