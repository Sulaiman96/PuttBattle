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

### D-6 — Consolidate editor MCP onto `ue-mcp` (drop UnrealClaude + VibeUE)   (2026-06-13)
**Context:** The two-server setup (UnrealClaude `unreal` + VibeUE `vibeue`) was fragile: VibeUE would not
stay connected (its in-editor service + API-key remote bridge kept dropping; after an editor restart the
session's MCP clients couldn't re-register its tools at all), blocking the Phase 1–2 editor content pass.
Both were young single-maintainer plugins. The `ue-mcp` fallback was pre-agreed in `MCP-SETUP.md`.
**Decision:** Execute the consolidation — delete both vendored plugins and all their config/doc references,
switch `.mcp.json` to a single `ue-mcp` server (TS server + `UE_MCP_Bridge` C++ plugin, `ws://localhost:9877`).
**Why:** One actively-maintained server covers both domains (levels/actors + Blueprints/materials/assets),
removing the VibeUE key/remote-bridge moving parts and the two-server tool-choice ambiguity. The trigger
condition in the old fallback plan ("a plugin broken for >1 task") was met. Rejected: keep fighting the
VibeUE connection (sunk-cost; root cause is upstream).
**Constraints carried forward:** vendor `Plugins/UE_MCP_Bridge` (Source + `.uplugin`, commit) like the old
plugins.
**Touches:** `.mcp.json`, `PuttBattle.uproject`, `CLAUDE.md` §11, `docs/CONVENTIONS.md` §11,
`docs/MCP-SETUP.md`, `docs/USER-ACTIONS.md` (UA-4…7), `docs/repo-root-files/*`, `Plugins/`.

**Resolution (2026-06-13) — two gotchas fixed during install:**
1. **Windows can't spawn `npx` MCP servers.** The wizard wrote `"command": "npx"`; Claude Code (Node ≥ 22)
   failed to connect — `npx` is a `.cmd` shim Node won't spawn (`ENOENT`/`EINVAL`), and `cmd /c npx`
   doesn't forward the child's stdio. Verified by spawning each form like Claude does. **Fix:** invoke
   `node` directly on the entry — `node node_modules/ue-mcp/dist/index.js PuttBattle.uproject` — which
   spawns cleanly and starts in ~250 ms. `ue-mcp` is pinned as a `devDependency` (repo `package.json`,
   `node_modules/` git-ignored) so `npm install` restores it. This is also, in hindsight, why the old
   `npx`-based `vibeue` never connected while the `node`-based `unreal` did.
2. **GAS got force-enabled by the bridge, and that's OK.** `UE_MCP_Bridge.uplugin` hard-depends on
   `GameplayAbilities`, so it can't be removed without breaking the tool. D22 is satisfied a different way:
   `ue-mcp.yml` disables the `gas` tool category (no GAS authoring) and **no game module** uses GAS. GAS is
   enabled-but-unused, editor-tool-only. (`PCG`/`foliage` categories likewise disabled.)

### D-7 — Player camera = 3 third-person follow presets (1/2/3), superseding D4's fixed rigs   (2026-06-13)
**Context:** Reviewing the graybox, the human (design authority) directed the player camera to be **three
ball-following third-person presets** — zoomed-in, mid (diagonal between top-down and third-person), and
top-down-angle — switched with **1/2/3**. This diverges from README **D4** ("Top-down/isometric; 2–4 fixed
rigs per hole variant, player cycles") and the C++ `PBCameraSubsystem`/`PBCameraRig` built for it (cycled
with C).
**Decision:** Implement the player camera as a `SpringArmComponent` + `CameraComponent` on `BP_BallPawn`
(boom uses **absolute rotation** so the rolling ball's spin doesn't tumble the view; arm 650). Three presets
switched by `IA_Camera1/2/3` (keys 1/2/3) handled in the pawn's event graph, each setting boom pitch + camera
FOV: P1 −28°/50, P2 (default) −45°/75, P3 −72°/88. Removed the three fixed `PBCameraRig` actors from
`H_Test/V_A` and the `C` cycle mapping from `IMC_Putt`. Kept the C++ `PBCameraRig`/`PBCameraSubsystem` classes
in the codebase (currently unused).
**Why:** A feel/visual call the human owns (CONVENTIONS §12). A spring-arm follow rig is the idiomatic
third-person approach and keeps framing values feel-tunable without recompiling (§1: camera is client
presentation, §2). Zoom is done via **FOV** (not runtime arm-length) because setting `TargetArmLength` at
runtime isn't reachable through the BP-authoring path available via ue-mcp; acceptable for graybox, can move
to C++ arm-length presets later if true dolly-zoom is wanted.
**Open / to reconcile (flagged for the human):**
- **README D4** should be updated to reflect the new player-camera model (left to the human as design-doc
  owner).
- The **hole-intro "camera glance" (D20)** and the **spectator system's "cycle that hole's camera rigs"**
  (Phase 8) both assumed fixed rigs. Revisit when those phases land — either keep per-hole rigs for those
  purposes alongside the player follow-cam, or redesign them around the follow-cam.
**Touches:** `Content/Ball/BP_BallPawn` (camera components + 1/2/3 graph), `Content/Input/IMC_Putt`
(+`IA_Camera1/2/3`, −`C`), `Content/Maps/Holes/H_Test/V_A` (rigs removed), `docs/plans/01-ball-and-shot.md`,
`docs/pr/phase-01-ball-and-shot.md`.

### D-8 — Default maps repointed to V_A + project-wide default GameMode (review fix)   (2026-06-15)
**Context:** A review of `phase/01-02-ball-shot-surfaces` found `EditorStartupMap`/`GameDefaultMap`/
`ServerDefaultMap` all pointing at `/Game/Maps/Core/L_Bootstrap`, which was never created (a still-open
Phase-0 deferral) — so the editor opened to an empty world and any non-PIE launch had no valid startup map.
Separately, `BP_PBGameMode` was set only as `V_A`'s per-map World Settings override, with no
`GlobalDefaultGameMode`; any map missing that override would run the engine default `AGameMode`, making
`APBGameMode::RespawnAtCheckpoint` (reached from `FellOutOfWorld`/hazards) silently no-op — the D7
water/void/KillZ reset would fail and the ball would be destroyed instead.
**Decision:** Point the three `GameMapsSettings` map keys at the only playable map
(`/Game/Maps/Holes/H_Test/V_A.V_A`) and set `GlobalDefaultGameMode=/Game/Match/BP_PBGameMode.BP_PBGameMode_C`.
**Why:** Smallest change that makes the project open into a valid world and makes the D7 contract hold on
every map (a per-map override still wins where present). Creating the real `L_Bootstrap`/front-end map stays
a Phase-0/3 deferral; revert the map keys to it once it ships.
**Touches:** `Config/DefaultEngine.ini`.

### D-9 — Surface override stack rooted via USTRUCT (review fix)   (2026-06-15)
**Context:** `UPBSurfaceSubsystem` held pushed overrides in a bare `TArray<FOverrideEntry>` whose
`TObjectPtr<UPBSurfaceDefinition>` was invisible to GC (a plain struct field is not a reflected reference),
so the definition governing an active Ice Age override could be collected mid-effect — the exact future
caller (Phase 6) the stack exists for.
**Decision:** Promote `FOverrideEntry` to a `USTRUCT` (`FPBSurfaceOverride`) with a `UPROPERTY()` Definition
and mark the array `UPROPERTY(Transient)`, matching the cup/GameMode container pattern.
**Why:** Makes the held definitions GC-reachable while they govern play; behaviour-neutral today.
**Touches:** `Surfaces/PBSurfaceSubsystem.{h,cpp}`.

### D-10 — Subclass the *Base* framework quartet, not the legacy MatchState classes   (2026-06-17)
**Context:** Phase 3 (T3.2) needs a GameMode/GameState that we drive through `EPBMatchPhase`. UE offers
`AGameMode`/`AGameState` (which carry Epic's auto-driven `MatchState` FSM: WaitingToStart→InProgress→…)
and the leaner `AGameModeBase`/`AGameStateBase`.
**Decision:** `APBMatchGameMode : AGameModeBase`, `APBMatchGameState : AGameStateBase`,
`APBLobbyGameMode : AGameModeBase`, `APBLobbyGameState : AGameStateBase`. Verified against 5.7 engine source.
**Why:** We own every phase transition; the legacy MatchState machine would run its own state changes and
fight ours. The Base classes give the replicated-quartet plumbing without the competing FSM. (Also keeps
the door shut on accidentally relying on `StartMatch()`/`MatchState` semantics that don't match our timer model.)
**Touches:** `Match/PBMatchGameMode.*`, `Match/PBMatchGameState.*`, `Match/PBLobbyGameMode.*`, `Match/PBLobbyGameState.*`.

### D-11 — Match/lobby GameModes are forced via the `?game=` travel URL, not by changing map settings   (2026-06-17)
**Context:** The multiplayer flow must run `APBMatchGameMode` on the hole map, but `V_A`'s World Settings
GameMode override is `BP_PBGameMode` (the offline referee), which the human is actively using for Phase 1–2
**feel-testing**. Changing `GlobalDefaultGameMode` or `V_A`'s World Settings to the match mode would gate
input behind the phase machine and break that testing.
**Decision:** Drive both hops with a `?game=/Script/PuttBattle.PB(Lobby|Match)GameMode` URL option
(`UPBSessionSubsystem` lobby travel; `APBLobbyGameMode::RequestStartMatch` match travel). Confirmed in
`UGameInstance::CreateGameModeForURL` (GameInstance.cpp:1522-1545) that a URL `?game=` overrides the map's
World Settings `DefaultGameMode`. The shot-gating code is also written to stay permissive when there is **no**
`APBMatchGameState` (offline), so `V_A` keeps working unchanged for feel-testing.
**Why:** Smallest change that adds the match flow without touching the maps the human is iterating on, and
without an editor round-trip (the bridge was down this session). No `GlobalDefaultGameMode`/World-Settings edits.
**Touches:** `Net/PBSessionSubsystem.*`, `Match/PBLobbyGameMode.*`, `Shot/PBShotComponent.cpp` (permissive offline gating).

**Update (2026-06-17, editor pass):** Refined the split once the lobby content existed. The **lobby** hop no
longer forces `?game=` — `L_Lobby`'s own World Settings GameMode is `BP_PBLobbyGameMode` (which carries the
lobby HUD), so host + joining clients share one lobby mode/HUD reliably (a runtime `?game=` to a BP path was
fragile to set from BP, and a World-Settings GameMode is saved-in-map and dependable). The **match** hop
(`APBLobbyGameMode::RequestStartMatch`) still forces `?game=/Script/PuttBattle.PBMatchGameMode` — that's the
part that keeps the shared graybox map `V_A` on its offline referee for feel-testing. `GameDefaultMap` was
repointed from `V_A` (the D-8 stopgap) to `L_Lobby` so launches land on the front-end menu; `V_A`'s own World
Settings stay on `BP_PBGameMode`. The now-unused `UPBSessionSubsystem::LobbyGameModeURL` is left in place
(harmless) pending a tidy-up build.

### D-12 — Ball replication cadence is a net-rate throttle, not true net dormancy   (2026-06-17)
**Context:** plans/03 T3.1 asks for "30 Hz while moving, 5 Hz at rest via dormancy-ish toggling". The 5.7
research flagged that `SetNetDormancy(DORM_DormantAll)` on an actively physics-replicated body can freeze a
ball mid-motion on clients if a **non-shot** force (boost volume, future Gust/Tornado) moves a dormant body.
**Decision:** Throttle `SetNetUpdateFrequency` (30 Hz moving / 5 Hz at rest, both `EditAnywhere`) from the
ball's own motion each server tick instead of toggling dormancy. Smoothing is `EPhysicsReplicationMode::
PredictiveInterpolation` + `bReplicatePhysicsToAutonomousProxy`.
**Why:** Achieves the bandwidth goal literally and safely; avoids the dormancy freeze-mid-motion failure mode
for a body that has force sources other than the player's shot. "dormancy-ish" in the plan signalled an
approximation was acceptable.
**Touches:** `Ball/PBBallPawn.{h,cpp}`.

### D-13 — Strokes + checkpoint tracking live on APBPlayerState; the base GameMode adopts it   (2026-06-17)
**Context:** Shots became a server RPC (T3.1), so a client's local `UPBShotComponent` no longer runs
`ExecuteShot` — a stroke counter on the component would never advance on clients. Checkpoints likewise must be
server-authoritative and per-player (T3.3), replacing the base mode's `TMap<BallPawn,Checkpoint>`.
**Decision:** `APBPlayerState` owns `Strokes` (replicated, server-incremented in `ExecuteShot`) and
`CheckpointIndex` (server-set on activation; respawn resolves the actor by index). The HUD binds the
PlayerState's `OnStrokesChanged`. The **base** `APBGameMode` now sets `PlayerStateClass = APBPlayerState` so
offline play and the match share one PlayerState and one checkpoint path.
**Why:** Single server-authoritative source of per-player truth; the offline feel-test map keeps working
(strokes still advance because the host is authority and runs `ExecuteShot` locally). Avoids a second
PlayerState class.
**Touches:** `Match/PBPlayerState.*`, `Match/PBGameMode.*`, `Shot/PBShotComponent.*`, `UI/PBHUDWidget.*`, `Course/PBCheckpointActor` (unchanged call site).
