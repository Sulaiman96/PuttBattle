# PUTT BATTLE — Project Conventions (repo CLAUDE.md)

You are working on **Putt Battle**, a 2–6 player online mini-golf battle game. UE **5.7**, C++ module `PuttBattle`, Steam, listen-server. The design authority is `docs/README.md`. Read the relevant `docs/plans/*.md` for your current task before writing code. Do not refactor outside your task's scope. If a task seems to require violating a rule below, stop and flag it instead of improvising.

## 1. Tech & layout
- **Hybrid C++/Blueprint boundary (D21):** C++ owns systems, replication, match flow, scoring math, RPC validation, and physics/collision logic — these are NEVER implemented in Blueprint. Blueprint owns: content wiring (subclassing C++ bases to plug meshes/curves/VFX), UMG widget visuals, and **simple powerup effect implementations** via the sanctioned BP effect API (§ plans/05 T5.2). Data Assets own all content/tuning values.
- **Feel-tuning surface rule:** any constant that affects game feel (damping, power curves, impulse magnitudes, drag multipliers, effect durations/forces) must be reachable without recompiling — as `EditAnywhere/BlueprintReadWrite` UPROPERTYs on the BP subclass, curve/Data Assets, or the `pb.*` console-variable set for live in-PIE tweaking. If tuning a feel value requires a C++ edit, that's a bug in the code's design.
- Source layout: `Source/PuttBattle/{Ball, Shot, Powerups, Surfaces, Course, Match, Net, Camera, Spectate, Progression, UI}` — one subsystem per folder, matching `Content/{Data, Maps/Core, Maps/Holes, Cosmetics, VFX, UI}`.
- Class prefix: project classes use `PB` after the UE prefix — `APBBallPawn`, `UPBShotComponent`, `FPBShotRequest`, `EPBMatchPhase`.
- Single runtime module. Do not create new modules without an explicit task instruction.

## 2. Authority & replication rules
- The server is authoritative over: shots, physics outcomes, powerup effects, timers, scoring, checkpoints, hole completion. Clients are authoritative over: camera, aim preview rendering, cosmetic VFX.
- RPC naming: `Server_*` (WithValidation where input is player-supplied), `Client_*`, `Multicast_*`. Every `Server_*` RPC validates: correct match phase, ball-at-rest where relevant, clamped numeric ranges, ownership.
- Replicated state lives in `AGameStateBase`/`APlayerState` subclasses, never in widgets or controllers. UI reads replicated state; UI never computes game truth.
- `OnRep_*` functions only update presentation. Game logic in OnRep is a bug.

## 3. The Ball Attributes pattern (critical)
All gameplay modifiers to a ball go through the replicated struct `FPBBallAttributes` on `APBBallPawn`:
```cpp
struct FPBBallAttributes {
  float PowerMultiplier = 1.f;     // Glue, Balloon
  float ScaleMultiplier = 1.f;     // Balloon
  bool  bAimIndicatorHidden = false; // Jumble
  bool  bAimInverted = false;      // Reverse
  bool  bShielded = false;         // Shield
  bool  bShotLocked = false;       // Lock
  bool  bGhostShotArmed = false;   // Ghost Ball
  bool  bAnchorArmed = false;      // Anchor
};
```
Powerup effects mutate attributes **server-side** via the effect runtime; the pawn applies them in one place (`ApplyAttributes()`). Never add an ad-hoc replicated bool to the pawn for a new effect — extend this struct.

## 4. Collision contract (never violate)
Object channels (defined in T0.1, `DefaultEngine.ini`):
- `PB_Ball` — balls. Profile `PB_BallProfile`: **ignores** `PB_Ball` (ghost balls), blocks `PB_Floor` + `PB_Wall`, overlaps triggers.
- `PB_Floor` — every surface a ball may roll on or come to rest on.
- `PB_Wall` — every blocking surface a ball should bounce off but never rest on.

**Level-authoring law:** every static mesh in every hole variant is assigned `PB_Floor` or `PB_Wall` — never default channels. Ghost Ball (passes through `PB_Wall` only) and rest-validity checks depend on this. The variant validator (plans/07) fails any map that breaks it.

## 5. GameplayTags
Taxonomy in `Config/DefaultGameplayTags.ini`:
```
Powerup.{Mulligan|Anchor|GhostBall|Teleport|Shield|Jumble|Balloon|Reverse|Shuffle|Swap|Lock|Ink|Glue|Gust|Tornado|IceAge|Bumper}
Surface.{Fairway|Ice|Sand|Boost|Sticky|Mud}
Hazard.{Water|Void}
Match.Phase.{Lobby|HoleIntro|Countdown|Playing|ShotGrace|HoleResults|MatchResults}
Effect.State.{Shielded|Ballooned|Jumbled|Reversed|Locked|Inked|Glued}
Unlock.{...}
```
New content adds tags here, never string literals. Tags are the identity keys for definitions, effects, save data, and achievements.

## 6. Data-asset workflow
- Definitions are `UPrimaryDataAsset` subclasses in `Content/Data/{Powerups|Surfaces|Holes|Scoring|Cosmetics|Achievements}`.
- Adding a powerup = 1 definition asset + 1 `UPBPowerupEffect` subclass + 1 tag. Adding a surface = 1 `UPBPhysicalMaterial` + 1 `UPBSurfaceDefinition` + 1 tag, **zero code**. Adding a hole variant = 1 Level Instance + 1 array entry.
- Tuning values never live in C++ constants; they live in config assets (`UPBScoringConfig`, curves) or `FPBMatchSettings`.

## 7. Timers & phases (reference)
`EPBMatchPhase`: Lobby → HoleIntro(4s glance) → Countdown(3s) → Playing(MapTime; first sink clamps remaining to FirstFinishClamp) → ShotGrace(pre-expiry shots simulate to rest, may score) → HoleResults(8s) → …repeat… → MatchResults. DNF = 0 points, never negative. The GameMode owns phase transitions; GameState replicates phase + clock.

## 8. Testing & Definition of Done
A task is done only when:
1. Project compiles with zero new warnings.
2. Pure-logic systems (scoring, weights, streak math, slot selection) have Automation Spec tests (`PuttBattle.Tests.*`) and they pass.
3. Multiplayer-touching work is verified in PIE with **3 clients + listen server** and net emulation (`p.NetEmulation` 100 ms latency / 1% loss) — remote balls smooth, no authority warnings.
4. The acceptance line in the task's plan file passes.
5. Any new tag/channel/asset path is reflected in this file or the plan file.

## 9. Git
Git + LFS (`*.uasset, *.umap, *.png, *.fbx, *.wav` in `.gitattributes`). Never commit `Intermediate/`, `Saved/`, `DerivedDataCache/`, `Binaries/`. Branch per phase (`phase/03-multiplayer-core`); conventional commits (`feat:`, `fix:`, `test:`, `content:`). Binary assets: one author at a time — coordinate before touching shared maps. **Never `git push`** — the human reviews and pushes. **No commit trailers** (no `Co-Authored-By`); keep messages concise.
- **PR summary per part:** every plan part/phase you implement gets a markdown summary at `docs/pr/phase-NN-<name>.md` (what shipped + where, verification evidence, known issues, and any remaining human/editor steps), written before that part's final commit. Mark the matching `docs/plans/*.md` tasks with status + date in the same pass.

## 10. Agent operating rules
- Read this file + your task's plan file before any code. Confirm acceptance criteria first; build toward them.
- Prefer the smallest change that satisfies the acceptance line. No speculative abstractions, no extra options "for later" — the extension points are already designed.
- **Do not introduce the Gameplay Ability System (GAS)** or other ability/effect plugins. The custom powerup framework (plans/05) is a deliberate decision (D22) — we borrow GAS's patterns (tags, data-driven definitions, central attributes, effect lifetimes) without its machinery. Suggesting GAS in a task is a scope violation.
- If an API you remember doesn't exist in UE 5.7, check the engine source in `Engine/Source` rather than guessing.
- When a plan file and the code disagree with README.md, README wins; record the conflict in `docs/DECISIONS.md` with date and resolution.
- Steps referencing a **UA-number** belong to the human (`docs/USER-ACTIONS.md`). Never perform, simulate, or mark them done yourself — surface them as a checklist and stop. Never ask the human to do work that isn't UA-shaped (accounts, secrets, GUI-only, second machine, other humans, feel judgment); that work is yours.

## 11. Editor MCP tools (ue-mcp)
One MCP server, **`ue-mcp`**, is configured in `.mcp.json` (setup: `docs/MCP-SETUP.md`). It talks to the `UE_MCP_Bridge` plugin in the open editor and covers **both** level/actor work and Blueprint/material/asset/Niagara/UMG work. Tools are **category + action** shaped — e.g. `project(action="get_status")`, `level(action="get_outliner")`, `blueprint(...)`, `material(...)`, `asset(...)`. Consolidated from the former two-server (UnrealClaude + VibeUE) setup — see `docs/DECISIONS.md` D-6.

Rules (non-negotiable):
1. **Editor-touching task? Check first.** Call `project` → `get_status` at session start. If the editor is closed or the bridge is down, do NOT stall or improvise: complete the C++/data/asset-definition work, then list the editor-side steps for the human under a "Requires editor" heading.
2. **Discover before you assume.** ue-mcp tools are action-dispatched with many actions per category; inspect a tool's available actions/parameters before invoking, and read state back (get before set). Never guess action names or required params.
3. **Full asset paths** (`/Game/Data/Powerups/DA_Gust`), never bare names. **Compile a Blueprint before adding variable/graph nodes that reference its members.**
4. **One asset-lifecycle path:** use ue-mcp's asset operations (create/save/move/duplicate/delete/import) for all asset lifecycle — never ad-hoc; `move` for renames, never duplicate+delete.
5. **Destructive operations** (delete asset/actor, overwrite a map) only when the current task explicitly names that thing. "Clean up" is never a deletion licence. Any map edit → re-run the variant validator (plans/07).
6. **Verify visually**: after scene/Blueprint changes, capture the viewport (or read logs) and confirm the result rather than assuming success.
7. **The repo is the source of truth**: MCP-created assets must land in the `Content/` layout per §1 and be committed; delete transient experiments before ending the session.
8. The tool allow-list and any auto-approve are **human decisions** — never edit `.claude/settings.json` permissions as part of a task. Never enable **GAS/GameplayAbilities** via the bridge or the `ue-mcp` wizard (D22).
9. For UE 5.7 API questions, prefer the engine source in `Engine/Source` (§10) over guessing — ue-mcp has no bundled 5.7 doc oracle.

## 12. Working with the human (read this — it shapes every interaction)
The human is a **senior C#/.NET engineer** and a **novice at game development and Unreal Engine**, deliberately learning both through this project. Consequences:
- **Whenever you pause for a UA item or ask them to do anything**, give exact, assume-nothing navigation: precise menu paths ("Edit → Plugins → search 'VibeUE' → tick Enabled → Restart Now"), what the screen should show, the expected result, and how to verify success. Never say "enable the plugin" or "check Project Settings" bare. General software literacy is assumed (terminals, Git, env vars need no hand-holding); Unreal, editor UI, and gamedev jargon may never be assumed.
- **Teach as you go, cheaply:** the first time a task uses an Unreal concept, add a 1–3 line entry to `docs/LEARNING.md` (append-only glossary) and reference it in the task summary. Use C#/.NET analogies where they genuinely fit (UPROPERTY ≈ attributes + reflection; modules ≈ csproj assemblies; UObject GC ≈ CLR GC with opt-in rooting). Never re-explain a concept already in LEARNING.md — link it.
- Prefer *why* over *what* in summaries: they can read the code; what they can't yet judge is whether an approach is idiomatic Unreal. One sentence of "the standard UE way here is X because Y" beats restating the diff.
- Never condescend on engineering fundamentals; never assume engine familiarity. Answer questions at senior-engineer depth with novice-Unreal vocabulary.
