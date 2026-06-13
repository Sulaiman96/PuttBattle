# PUTT BATTLE — Decision Log

Append-only record of design/architecture decisions and resolved conflicts.
Per CONVENTIONS §10: when a plan file and the code disagree with `README.md`,
README wins — record the conflict here with date and resolution. Also log any
deliberate deviation from a plan, and the rationale.

Foundational decisions (D1…D22) are described in `docs/README.md`; this file is
the running log of decisions made *during implementation*.

## Format

Each entry:

```
### D-<n> — <short title>   (YYYY-MM-DD)
**Context:** what forced a decision (conflict, ambiguity, trade-off).
**Decision:** what we chose.
**Why:** the reasoning, including options rejected.
**Touches:** files / plans / tags / channels affected.
```

---

<!-- New entries below, newest last. -->

### D-1 — Framework classes (GameMode, PlayerController) live in `Match/`   (2026-06-13)
**Context:** CONVENTIONS §1 fixes the subsystem folder list; `APBGameMode`/`APBPlayerController` (the
"framework quartet" with GameState/PlayerState) have no folder of their own, and Phase 1 needs both.
**Decision:** Put them in `Source/PuttBattle/Match/`.
**Why:** Match is where GameState/PlayerState and the phase machine land in Phases 3–4, so the quartet
stays together. Rejected: a new `Player/` folder (would violate the §1 layout) and `Shot/` (too narrow —
the controller also drives cameras and, later, powerups/spectate).
**Touches:** `Match/PBGameMode.*`, `Match/PBPlayerController.*`.

### D-2 — Surface gameplay hook is an enum, not a GameplayTag   (2026-06-13)
**Context:** plans/02 T2.1 lists a `GameplayHookTag` (none/boost/sticky/hazard) on the surface
definition, but those four values aren't in the §5 taxonomy.
**Decision:** Model it as `EPBSurfaceHook { None, Boost, Sticky, Hazard }` rather than inventing tags.
**Why:** CONVENTIONS §5 says new content adds tags to the taxonomy, never ad-hoc — these are an internal
behaviour classification, not a content identity key (Boost is realised by `PBBoostVolume`, Hazard by
`PBHazardVolume`). An enum avoids tag sprawl. The surface's *identity* still uses `SurfaceTag` (Surface.*).
**Touches:** `Surfaces/PBSurfaceDefinition.h`.

### D-3 — Module root on the include path for the flat subsystem layout   (2026-06-13)
**Context:** Headers are included by sub-path (`"Ball/PBBallPawn.h"`); UBT only auto-adds Public/Private,
which our flat layout doesn't have, so cross-folder includes failed to compile.
**Decision:** `PublicIncludePaths.Add(ModuleDirectory)` in `PuttBattle.Build.cs`.
**Why:** Keeps the §1 one-folder-per-subsystem layout while making every header includable by its path
from the module root. Rejected: adding every subfolder individually (noisier, and bare filenames invite
collisions).
**Touches:** `PuttBattle.Build.cs`.

### D-4 — Ball mass scales with volume; balloon "floatiness" is later tuning   (2026-06-13)
**Context:** T1.1 wants a console scale change to visibly update physics mass; Balloon (Phase 6) also
wants a "floatier" feel.
**Decision:** `ApplyAttributes()` sets mass = `BaseMassKg × ScaleMultiplier³` (density-preserving).
**Why:** Makes `pb.Ball.SetScale 1.6` change mass observably (the T1.1 acceptance check) with a physically
honest default. Balloon's floatiness is a power/damping tuning concern handled by its effect + curves in
Phase 6, not baked into the mass model now.
**Touches:** `Ball/PBBallPawn.cpp`.

### D-5 — Roll drag is a custom mass-independent acceleration; bounce stays with physical materials   (2026-06-13)
**Context:** T2.1 notes Chaos friction alone won't give controllable roll feel.
**Decision:** The sampler applies its own deceleration via `AddForce(..., bAccelChange=true)` scaled by the
surface's `RollDragMultiplier`; friction/restitution remain on the `UPBPhysicalMaterial` (mirrored from the
definition) for Chaos contacts/bounces.
**Why:** Decouples the two knobs — roll distance is tuned by one multiplier independent of ball mass, while
bounciness stays where the engine expects it. The pure model is `ComputeRollDeceleration`, shared with the
automation test so feel changes are regression-checked.
**Touches:** `Surfaces/PBSurfaceSamplerComponent.*`, `Surfaces/PBPhysicalMaterial.*`.
