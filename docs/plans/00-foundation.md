# Phase 0 — Foundation

**Context:** Empty start. Read `CONVENTIONS.md` fully first — this phase creates the things it references.

## T0.1 — Project bootstrap ✅ COMPLETED 2026-06-13
> Commit `749aaab`. Game target builds clean; Steam OSS (dev AppId 480), Steam NetDriver,
> `PB_Ball`/`PB_Floor`/`PB_Wall` + `PB_BallProfile` in `DefaultEngine.ini`; UE `.gitignore` + LFS
> `.gitattributes`. Deferred to T0.3 editor pass: `L_Bootstrap.umap` creation (config already
> points at it), packaged-build launch check, visual collision-channel confirmation.

**Goal:** A compiling, packaged-runnable UE 5.7 C++ project with networking-ready settings.
**Precondition (human):** UA-1/UA-2 complete — toolchain installed, blank C++ project `PuttBattle` created via the wizard.
**Create:**
- Verify the blank project opens and builds; correct any `.uproject`/config drift from the wizard.
- Enable plugins: Online Subsystem Steam, Enhanced Input (default on), Level Instance support is built-in.
- `PuttBattle.Build.cs` deps: `OnlineSubsystem`, `OnlineSubsystemSteam`, `OnlineSubsystemUtils`, `GameplayTags`, `UMG`, `Niagara`, `EnhancedInput`.
- `DefaultEngine.ini`: Steam OSS config with dev AppId **480** (Spacewar) for development; `NetDriver` definitions for Steam sockets; collision **object channels** `PB_Ball`, `PB_Floor`, `PB_Wall` and preset `PB_BallProfile` exactly per CONVENTIONS §4.
- Git repo: UE `.gitignore`, `.gitattributes` with LFS rules (CONVENTIONS §9), initial commit.
- `Maps/Core/L_Bootstrap` (empty), set as default map.
**Done when:** `Development` packaged build launches to the empty map; collision channels visible in Project Settings; repo clean after a fresh clone + build.

## T0.2 — Conventions, tags, input ✅ COMPLETED 2026-06-13
> Commit `8ac6073`. `CLAUDE.md` at root; `docs/` package committed (Docs→docs case normalized);
> full §5 tag taxonomy in `DefaultGameplayTags.ini`; 11 placeholder UCLASSes lock the §1 source
> layout; `DECISIONS.md` seeded; LEARNING.md entries added (collision channels, OSS/NetDriver,
> Enhanced Input). Deferred to T0.3 editor pass: `IMC_Putt` + `IA_*` input assets (.uassets),
> tag-picker visual check.

**Goal:** The guardrails every later agent session depends on.
**Create:**
- Copy `CONVENTIONS.md` → repo root as `CLAUDE.md`; copy `README.md` + `plans/` → `docs/`.
- `Config/DefaultGameplayTags.ini` seeded with the full taxonomy (CONVENTIONS §5).
- Enhanced Input: `IMC_Putt` with actions `IA_DragShot` (LMB press/hold/release), `IA_CycleCamera` (C), `IA_Powerup1/2/3` (1/2/3), `IA_SpectatePrev/Next` (←/→), `IA_CancelTargeting` (RMB/Esc).
- `docs/DECISIONS.md` (empty log with format header).
- Stub C++ skeleton folders per CONVENTIONS §1 with a placeholder class each so the layout is locked in.
**Done when:** project compiles; tags appear in editor tag picker; a fresh Claude Code session, given only the repo, correctly answers "where does a new powerup's code and asset go?"

## T0.3 — Editor MCP integration 🔶 AGENT WORK DONE 2026-06-13 — awaiting UA-4…7
> Branch `phase/00-foundation`. Both plugins vendored (inner `.git` stripped): UnrealClaude
> cloned `--recurse-submodules`, nested folder flattened so `Plugins/UnrealClaude/UnrealClaude.uplugin`
> + `Resources/mcp-bridge/index.js` resolve per `.mcp.json`; VibeUE verified complete. Bridge
> `npm install` done. `.mcp.json` + `.claude/settings.json` placed at root; VibeUE `@import`
> added to `CLAUDE.md`; both plugins enabled in `.uproject`. Remaining: human UA-4…7, then the
> MCP verification checklist + deferred editor items (L_Bootstrap map, IMC_Putt inputs, UA-8).

**Goal:** Every future Claude Code session controls the Unreal editor with zero per-session setup.
**Agent does:** clone both plugins into `Plugins/` (UnrealClaude with `--recurse-submodules`; fix its nested-folder layout per MCP-SETUP §1), `npm install` the bridge, build, vendor (strip inner `.git`, commit), place `.mcp.json` + `.claude/settings.json` at repo root, add the VibeUE rules import line to `CLAUDE.md`, then drive the MCP-SETUP verification checklist.
**Pauses for human (UA-4…7):** VibeUE API key + env var, editor GUI toggles (auto-approve OFF), first-run MCP server approval. Surface these as a short checklist and stop until confirmed.
**Done when:** the full MCP-SETUP verification passes — `/mcp` shows both servers connected, `unreal_status` returns project info, discovery proves the API key, and the spawn-screenshot-delete test triggers permission prompts for the non-allowlisted tools (human confirms the prompts: UA-8).
