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

### D-14 — Engine 5.7→5.8 upgrade; editor MCP switched from `ue-mcp` to the built-in "Unreal MCP"   (2026-06-18)
**Context:** The human upgraded the project to UE 5.8 (`EngineAssociation` in `PuttBattle.uproject`) to use the
first-party MCP server Epic ships in 5.8. Two blockers surfaced: (a) the targets still declared 5.7 build
settings, and (b) the vendored `UE_MCP_Bridge` plugin (the `ue-mcp` bridge — supported only to 5.7; latest npm
`1.0.80` still 5.4–5.7) **fails to compile on 5.8**: UE 5.8 changed `FJsonObject::Values` keys from `FString`
to `FStringType` and relocated `InstancedStruct.h` out of the deprecated `StructUtils` plugin (7 errors across
6 bridge handler files; **zero** errors in game code).
**Decision:** (1) Bump both target files to 5.8 settings (`BuildSettingsVersion.V7`,
`EngineIncludeOrderVersion.Unreal5_8`). (2) Disable `UE_MCP_Bridge` in `.uproject` (kept vendored for a future
5.8-capable upstream release) and enable Epic's built-in `ModelContextProtocol` plugin (Experimental;
auto-pulls `ToolsetRegistry` + `EngineAssetDefinitions`). Server `unreal-mcp` runs HTTP+SSE on
`http://127.0.0.1:8000/mcp`; `bAutoStartServer=True` set in `Config/DefaultEditorPerProjectUserSettings.ini`;
`.mcp.json` repointed from the ue-mcp WebSocket server to the built-in HTTP server.
**Why:** The human chose the first-party server (their reason for upgrading) over hand-patching vendored
third-party code that an upstream update would clobber. Bumping target settings (vs UBT's other suggested fix,
`TargetBuildEnvironment.Unique`, which would force recompiling the whole installed engine) is the standard
cross-version upgrade step and keeps the shared-binary installed-engine build.
**Trade-offs / carried forward:**
- The built-in plugin is **Experimental** and far narrower than ue-mcp (5 toolsets vs ue-mcp's 19 categories /
  400+ actions: no Niagara/StateTree/landscape/Blueprint-graph authoring). `CLAUDE.md §11` and the `ue-mcp-*`
  skills now describe an **inactive** tool and need rewriting for the built-in workflow (follow-up).
- `GameplayAbilities` is now **orphaned** — its only justification (D-6) was the `UE_MCP_Bridge` hard
  dependency. With the bridge off, nothing requires it; removable per D22 in a cleanup pass.
- Only one editor instance can bind `:8000` (Phase-3 two-instance test → 2nd instance has no MCP).
- **Revert path:** ~~re-enable `UE_MCP_Bridge` + restore the ue-mcp `.mcp.json` entry~~ — **superseded by
  D-15** (ue-mcp fully removed 2026-06-18). Reverting now means re-adding the bridge from upstream once it
  ships 5.8 support (or dropping back to 5.7).
**Verification (2026-06-18):** game + editor targets compile clean on 5.8; headless `-nullrhi` boot loads all
plugins (incl. `ModelContextProtocol`/`ToolsetRegistry`) and passes both `PuttBattle.Tests.*` specs; MCP server
binds `:8000` and `GET /mcp` → HTTP 405 (correct for a POST/SSE endpoint).
**Touches:** `PuttBattle.uproject`, `Source/PuttBattle.Target.cs`, `Source/PuttBattleEditor.Target.cs`,
`Config/DefaultEditorPerProjectUserSettings.ini`, `.mcp.json`, `docs/MCP-SETUP.md`, `CLAUDE.md` §11,
`docs/LEARNING.md`, `docs/pr/engine-58-upgrade.md`.

### D-15 — `ue-mcp` fully removed (revert path abandoned)   (2026-06-18)
**Context:** D-14 switched the active editor MCP to Epic's built-in `unreal-mcp` server but kept the
`ue-mcp` / `UE_MCP_Bridge` bridge vendored-but-disabled as a revert path. With 5.8 confirmed working on the
first-party server, the human decided not to keep the dead third-party bridge around — the leftovers
(disabled plugin, npm package, skills, settings hooks) only confuse tooling by pointing at tools that no
longer load.
**Decision:** Delete everything ue-mcp: the `Plugins/UE_MCP_Bridge/` source tree; the `ue-mcp` npm
devDependency (so `package.json` + `package-lock.json`, both ue-mcp-only) and `node_modules/`; `ue-mcp.yml`;
the four `.claude/skills/ue-mcp-*` skills; the disabled `UE_MCP_Bridge` entry in `PuttBattle.uproject`; and the
stale `ue-mcp`/`vibeue` references in the Claude settings + docs. Also swept the dead `Plugins/UnrealClaude` +
`Plugins/VibeUE` build shells (empty `Intermediate/` folders left behind by D-6, untracked). The built-in
`unreal-mcp` server is now the **sole** editor MCP.
**Why:** The revert path's only value — rolling back to ue-mcp's breadth — requires an upstream 5.8 release
that does not exist. Until it does, the vendored code is dead weight. Removing it makes the repo match reality;
if ue-mcp ever ships 5.8 support and we want it back, re-add it fresh from upstream (cleaner than resurrecting
a stale vendored copy). The built-in server's narrowness (no Niagara/Blueprint-graph/landscape authoring) is
the accepted cost, already noted in D-14.
**Carried forward:** ~~`GameplayAbilities` is still orphaned (its only justification was the bridge, D-6/D-14) —
removable per D22 in a separate pass.~~ **Done (2026-06-18):** the orphaned `GameplayAbilities` plugin entry was
removed from `PuttBattle.uproject`. Pre-removal audit confirmed zero use: no `GameplayAbilities`/`AbilitySystem`/
`UGameplayAbility`/`GameplayEffect`/`GameplayTasks` references in `Source/`, no `GameplayAbilities`/`GameplayTasks`
in any `.Build.cs` (the module's `GameplayTags` dependency is unrelated and stays), and no `Content`/`Config`
asset references — consistent with D22 (GAS never used in game code). **Needs an editor restart to take effect;**
the human should confirm the 5.8 editor still opens cleanly afterward — if it warns about a missing plugin
reference, some asset secretly touched GAS and this should be reverted and re-investigated.
**One human step:** `.claude/settings.json` still carries a never-firing
`mcp__ue-mcp__editor` PostToolUse hook; CONVENTIONS §11 forbids agents from editing that file's contents, so
the human deletes the `hooks` block.
**Touches:** `PuttBattle.uproject`, `Plugins/` (UE_MCP_Bridge + UnrealClaude + VibeUE removed), `package.json`
+ `package-lock.json` (deleted), `ue-mcp.yml` (deleted), `.claude/skills/ue-mcp-*` (deleted),
`.claude/settings.local.json`, `docs/repo-root-files/.mcp.json`, `CLAUDE.md` §11, `docs/CONVENTIONS.md` §11,
`docs/MCP-SETUP.md`, `docs/USER-ACTIONS.md` (T0.3 / UA-4…8), `docs/pr/engine-58-upgrade.md`.

### D-16 — `ue-mcp` restored and patched to compile on UE 5.8 (supersedes D-15)   (2026-06-18)
**Context:** After D-15 removed ue-mcp, the built-in `unreal-mcp` (D-14) proved too narrow to actually use: a
live `list_toolsets` exposed only `AgentSkillToolset` — no Blueprint or UMG widget editing, which is exactly
what debugging the lobby (`WBP_Lobby`) and Phase 5+ content work require. A capability survey found no
open-source 5.8 MCP with that breadth, and downgrading to 5.7 to use ue-mcp is unsafe (5.8-saved assets will
not open in 5.7 — silent null references / data loss). The human chose to bring ue-mcp back and fix it forward.
**Decision:** Restore the deleted ue-mcp from git (`git checkout 80b96ed~1 -- Plugins/UE_MCP_Bridge package.json
package-lock.json ue-mcp.yml .claude/skills/ue-mcp-*`), patch it for 5.8, and re-wire it as the active MCP. The
5.8 break surface (verified against the engine source — far smaller than feared) was exactly two classes:
(a) **StructUtils relocation** — `InstancedStruct.h` moved into CoreUObject, so the include in
`AnimationHandlers_StateMachine.cpp` was corrected and the now-dead `StructUtils` module dep (`Build.cs`) +
plugin entry (`.uplugin`) removed; (b) **`FJsonObject::Values` keys** changed `FString`→`UE::FSharedString`,
fixed at ~16 sites across 12 handler files with `const FString KeyStr(Pair.Key.ToView());` hoisted per
`->Values` loop (only the `== TEXT(...)`, `FString`/`const FString&` assignment, and TArray<FString>/TPair<FString>
insert sites genuinely break; raw `*Pair.Key` derefs still compile via `FSharedString::operator*()`).
**Result:** the editor target builds clean on 5.8 (`Result: Succeeded`, `UnrealEditor-UE_MCP_Bridge.dll` linked);
only C4996 deprecation warnings remain in vendored anim/gameplay/landscape/material handlers (tolerated, like
the pre-existing notices in D-14). Re-enabled `UE_MCP_Bridge` + its `GameplayAbilities` hard-dependency in
`PuttBattle.uproject` — re-introducing the D-6 GAS-enabled-but-unused exception and reversing D-15's GAS removal.
`.mcp.json` repointed from `unreal-mcp` (HTTP) back to `ue-mcp` (`node node_modules/ue-mcp/dist/index.js`);
`npm install` restores `node_modules`.
**Why not the shortcut:** rejected `UE_JSONOBJECT_LEGACY_STRING_KEYS=1` (a one-line revert of the key type) — on a
binary/installed engine it ABI-mismatches the precompiled `Json` module and risks silent memory corruption; it is
only safe on a fully source-built engine.
**Carried forward / open:** (1) runtime connection still to confirm — reopen the 5.8 editor + restart the Claude
session, then `project get_status`. (2) ue-mcp's **license** is ambiguous (its GitHub terms suggest a commercial
licence may be required for a shipping commercial game) — verify before release. (3) We now maintain a **fork** of
the bridge; an upstream 5.8 release would supersede these patches. (4) The built-in `ModelContextProtocol` /
`MCPClientToolset` plugins remain enabled in-editor but are not in `.mcp.json`. (5) `docs/MCP-SETUP.md`,
`docs/CONVENTIONS.md` §11, and `docs/USER-ACTIONS.md` still describe the removed/native state and need a sweep.
**Touches:** `PuttBattle.uproject`, `Plugins/UE_MCP_Bridge/**` (restored + patched), `package.json`,
`package-lock.json`, `ue-mcp.yml`, `.claude/skills/ue-mcp-*` (restored), `.mcp.json`, `CLAUDE.md` §11,
`docs/LEARNING.md` (pending), `docs/MCP-SETUP.md` (pending), `docs/CONVENTIONS.md` §11 (pending),
`docs/USER-ACTIONS.md` (pending).
