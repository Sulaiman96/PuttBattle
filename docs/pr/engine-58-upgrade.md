# Engine upgrade — UE 5.7 → 5.8 + built-in MCP switch  (2026-06-18)

Unplanned maintenance task (not a numbered phase). See `docs/DECISIONS.md` **D-14** for the decision record.

## What changed
- **`PuttBattle.uproject`** — `EngineAssociation` `5.7`→`5.8` (human); `UE_MCP_Bridge` plugin entry removed
  (the bridge was first disabled, then fully deleted — see **D-15**); Epic's `ModelContextProtocol` plugin enabled.
- **`Source/PuttBattle.Target.cs`, `Source/PuttBattleEditor.Target.cs`** — `BuildSettingsVersion.V6`→`V7`,
  `EngineIncludeOrderVersion.Unreal5_7`→`Unreal5_8`. Required: on an installed engine UBT forbids a target
  from modifying warning-level settings it shares with the precompiled engine, which the stale 5.7 settings
  did (unreachable-code / return-type / dangling). **No game source changes were needed.**
- **`Config/DefaultEditorPerProjectUserSettings.ini`** (new) —
  `[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]` `bAutoStartServer=True`, port 8000,
  path `/mcp`.
- **`.mcp.json`** — repointed from the `ue-mcp` WebSocket bridge to the built-in `unreal-mcp` HTTP server
  (`{ "type": "http", "url": "http://127.0.0.1:8000/mcp" }` — the exact shape Epic's
  `ModelContextProtocol.GenerateClientConfig ClaudeCode` writes).
- **Docs** — `docs/DECISIONS.md` D-14; superseded-banners on `docs/MCP-SETUP.md` + `CLAUDE.md §11`;
  `docs/LEARNING.md` engine-upgrade entry.

## Why built-in MCP (not patching ue-mcp)
`ue-mcp`/`UE_MCP_Bridge` does not compile on 5.8 (UE 5.8 changed `FJsonObject::Values` key type
`FString`→`FStringType`; moved `InstancedStruct.h` out of the deprecated `StructUtils` plugin) — 7 errors in
6 bridge handler files, and the latest npm `1.0.80` still targets 5.4–5.7. The human chose Epic's first-party
server (the reason for the upgrade) over hand-patching vendored third-party code an upstream update would
overwrite.

## Verification (all on UE 5.8, editor was closed)
- `PuttBattle` (Game) target: **compiles clean**.
- `PuttBattleEditor` target: **compiles clean** (bridge excluded).
- Headless boot (`UnrealEditor-Cmd … -nullrhi -unattended -ExecCmds="Automation RunTests PuttBattle.Tests"`):
  all plugins load incl. `ModelContextProtocol`/`ToolsetRegistry`; `PuttBattle.Tests.Shot.RequestValidation`
  ✅ and `PuttBattle.Tests.Surfaces.RollDistanceOrdering` ✅.
- MCP server: binds `127.0.0.1:8000`, `GET /mcp` → `HTTP/1.1 405` (correct — endpoint expects POST/SSE).

## Remaining human steps
1. **Open the UE 5.8 editor once.** First open recompiles shaders (slow, one-time); an editor-layout reset
   and asset-resave prompts on save are benign. Confirm the Output Log shows the MCP server starting on `:8000`.
2. **Restart the Claude Code session** so it loads the new `.mcp.json` and connects to `unreal-mcp` (the
   session that performed the upgrade still has the old `ue-mcp` server loaded).
3. Optional sanity check: `Edit ▸ Plugins` → search **"Unreal MCP"** → confirm it and **Toolset Registry**
   show **Enabled**.

## Known issues / follow-ups
- Built-in server is **Experimental** and much narrower than ue-mcp (5 toolsets vs 19 categories) — no
  Niagara/StateTree/landscape/Blueprint-graph authoring. `CLAUDE.md §11` was rewritten for the built-in
  workflow and the `ue-mcp-*` skills were removed (**D-15**).
- ~~`GameplayAbilities` is now orphaned (only the bridge needed it, D-6) — removable per D22.~~ **Removed
  2026-06-18** (D-15 carried-forward, now done): the plugin entry is gone from `PuttBattle.uproject` after an
  audit found zero use anywhere in `Source/`, `.Build.cs` deps, or `Content`/`Config`. **Takes effect on the
  next editor restart** — the human should confirm the editor opens cleanly (a "missing plugin reference"
  warning would mean an asset secretly used GAS → revert and investigate).
- One editor instance per `:8000` (Phase-3 two-instance Steam test → 2nd instance runs without MCP).
- ~~Revert path: re-enable `UE_MCP_Bridge` + restore its `.mcp.json` entry~~ — **ue-mcp was fully removed the
  same day (D-15)**; reverting now means re-adding the bridge from upstream once it ships 5.8 support.
