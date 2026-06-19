# Phase 7 — Course Variation

**Context:** Holes so far are hand-loaded test maps. This phase delivers D11 + the variant system, and the authoring validator that protects the collision contract.

## T7.1 — Definitions & CourseDirector
**Create:** `Course/PBHoleDefinition` (PrimaryDataAsset): SlotIndex, Par, `TArray<FPBHoleVariant>` { LevelInstance (TSoftObjectPtr<UWorld>), DifficultyTag, optional PickupPoolOverride }. `Course/PBCourseDefinition`: 9 ordered slots, theme, default powerup pool. `Course/PBCourseDirectorSubsystem` (server-authoritative, GameState-replicated results):
- At match start: seed (replicated) → if HoleCount < 9, select N distinct slots preserving order (DF4) → resolve one variant per chosen slot.
- Per hole: stream the variant Level Instance (server + clients via replicated resolved list), position TeePads/spawns, hand camera rigs to the camera subsystem, hand pickup spawn points to the pickup system, despawn previous.
**Done when:** same seed reproduces the same course on all clients; 1000-seed Automation test shows slot/variant selection is uniform and order-preserving; hole transitions stream cleanly with no stale actors (checkpoints, pickups, placed bumpers/tornados all cleared).

## T7.2 — Variant authoring contract + validator
**Create:** required-actor manifest per variant: 1 TeePad (with 6 spawn offsets), 1 CupActor, 2–4 CameraRigs, ≥1 PickupSpawn, KillZ/void coverage, optional Checkpoints/Hazards. **Editor validation** (editor module or commandlet `ValidateHoleVariants`): every static mesh on `PB_Floor`/`PB_Wall` (CONVENTIONS §4 — Ghost Ball depends on it), manifest satisfied, course-bounds volume present. Run in CI/pre-commit if available; otherwise a documented manual step in CONVENTIONS.
**Done when:** validator fails a deliberately broken map for each rule and passes the good ones.

## T7.3 — Graybox course content ∥
**Create:** 9 slots × 2 variants graybox (18 layouts), each using surfaces/hazards/checkpoints meaningfully; variants of a slot share an outer footprint but change internals (cost amortisation). Include at least: one ramp-jump hole, one Ghost-Ball-bait wall maze, one checkpoint-gated long hole, one open putting-duel green.
**Done when (agent):** full 9-hole matches run with no repeated layout; every variant passes the validator. Difficulty/fun review is human (UA-15); agent reworks flagged layouts afterwards.

**Progress (2026-06-19):** 2/18 layouts built.
- `H_03_CakeStand/V_A` — the *open putting-duel green* (Hole 3, PAR 3). See `docs/pr/phase-07-hole-03-cakestand.md`.
- `H_04_SlushieChute/V_A` — the *ice corridor / dogleg* (Hole 4, PAR 3), built by duplicating H_03 and re-shaping its already-channeled meshes (PB_Floor/PB_Wall carry over; `physMaterialOverride` is writable → Ice). Fully functional in-session. Omits the decorative alcove + left rail to fit H_03's 3-floor/4-wall channeled budget. See `docs/pr/phase-07-hole-04-slushiechute.md`.

Open course-wide: Lolly beacons sit on `ECC_WorldStatic` (§4) — the native editor MCP can't set component collision channels (only the old `execute_python` ue-mcp / editor can); finish all beacons to PB_Wall (or NoCollision) in one pass. T7.3 not complete (2 of 18 layouts).

## T7.4 — Variant reveal
**Create:** HoleIntro presentation: camera glance pan along the hole (rig-to-rig sweep or authored spline per variant), hole name/number + par card, into the 3-2-1 countdown (D20).
**Done when:** flow feels like SBG's hole reveal; total intro ≤ 7 s.
