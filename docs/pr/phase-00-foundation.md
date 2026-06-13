# PR: Phase 0 — Foundation (T0.1–T0.3)

**Branch:** `phase/00-foundation` · **Date:** 2026-06-13 · **Status:** agent work complete; T0.3 awaits human steps UA-4…7 before the MCP verification checklist can run.

## What this delivers

Phase 0 of `docs/plans/00-foundation.md`: a compiling, networking-ready UE 5.7 project with the
conventions, tags, and editor-MCP plumbing every later agent session depends on.

### T0.1 — Project bootstrap (commit `749aaab`, on `main`)
- `.uproject`: enabled OnlineSubsystem(+Steam/Utils), Niagara, EnhancedInput.
- `PuttBattle.Build.cs`: added OnlineSubsystem(+Steam/Utils), GameplayTags, UMG, Niagara, EnhancedInput.
- `DefaultEngine.ini`: Steam OSS (dev AppId 480 / Spacewar), SteamNetDriver with IpNetDriver fallback,
  collision object channels `PB_Ball`/`PB_Floor`/`PB_Wall` + `PB_BallProfile` preset (CONVENTIONS §4),
  default map pointed at `/Game/Maps/Core/L_Bootstrap`.
- UE `.gitignore` (replaces the GitHub stub) + LFS `.gitattributes` (CONVENTIONS §9).
- Repo wired to `github.com/Sulaiman96/PuttBattle`, based on the existing stub commit — no force-push.

### T0.2 — Conventions, tags, skeleton (commit `8ac6073`, on `main`)
- `CLAUDE.md` = `CONVENTIONS.md` at repo root; `docs/` package committed (`Docs`→`docs` normalized
  for case-sensitive clones); `docs/DECISIONS.md` log seeded.
- `Config/DefaultGameplayTags.ini`: full §5 taxonomy — 17 powerups, 6 surfaces, 2 hazards,
  7 match phases, 7 effect states.
- 11 placeholder `UCLASS`es lock in the §1 source layout
  (`Ball, Shot, Powerups, Surfaces, Course, Match, Net, Camera, Spectate, Progression, UI`).
- `docs/LEARNING.md`: entries for collision channels/profiles, OSS/NetDriver, Enhanced Input.

### T0.3 — Editor MCP integration (this branch)
- **UnrealClaude v1.5** cloned `--recurse-submodules`; the upstream nested layout flattened so
  `Plugins/UnrealClaude/UnrealClaude.uplugin` and `Resources/mcp-bridge/index.js` sit where
  `.mcp.json` expects; bridge `npm install` run (`@modelcontextprotocol/sdk`); `node --check index.js` passes.
- **VibeUE v4.0** (pre-cloned) verified complete (`.uplugin`, Source, `Content/samples/AGENTS.md.sample`).
- **Vendored both**: inner `.git`/`.gitmodules` stripped — versions pinned, fresh clone works offline.
- Both plugins enabled in `PuttBattle.uproject`; both target EngineVersion 5.7.0.
- `.mcp.json` + `.claude/settings.json` placed at repo root (from `docs/repo-root-files/`);
  VibeUE agent-rules `@import` added to `CLAUDE.md` (MCP-SETUP §6).

## Verification evidence
- `Build.bat PuttBattleEditor Win64 Development` → **Result: Succeeded** (77 actions; both editor
  plugin modules compiled and linked). Game target also builds.
- Staging audited each commit: no `Binaries/`, `Intermediate/`, `Saved/`, `node_modules/`, `.sln`.

## Known issues / notes
- **2 upstream warnings** from VibeUE's `.uplugin` (undeclared `GameplayTagsEditor` / `StructUtils`
  plugin deps). Left unpatched deliberately to keep the vendored copy identical to upstream v4.0.
- Deferred editor-side items (need a running editor + MCP, after UA-4…7): create `L_Bootstrap.umap`,
  `IMC_Putt` + `IA_*` input assets, packaged-build launch check, collision-channel + tag-picker
  visual confirmation, full MCP verification checklist (MCP-SETUP), UA-8 guardrail test.

## Human actions required next
UA-4 (VibeUE key) → UA-5 (env var + editor paste) → UA-6 (plugin/GUI checks) → UA-7 (MCP server
approval on `claude` launch). See `docs/USER-ACTIONS.md`; exact walkthrough provided in session.
