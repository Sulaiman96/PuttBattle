# PR: Restore ue-mcp + patch UE_MCP_Bridge for UE 5.8  (2026-06-18)

Unplanned maintenance (not a numbered phase). Decision record: `docs/DECISIONS.md` **D-16** (supersedes D-15).

## Why
The 5.8 upgrade (D-14) switched to Epic's built-in `unreal-mcp` and D-15 removed ue-mcp. In practice the built-in
server was too narrow — a live `list_toolsets` exposed only `AgentSkillToolset`, with **no Blueprint or UMG widget
editing** (needed to debug `WBP_Lobby` and for Phase 5+ content). No open-source 5.8 MCP offers that breadth, and
downgrading to 5.7 would strand 5.8-saved assets. So we restored ue-mcp and fixed it forward to 5.8.

## What shipped
- **Restored** from git (`80b96ed~1`): `Plugins/UE_MCP_Bridge/` (86 files), `package.json`/`package-lock.json`,
  `ue-mcp.yml`, the four `.claude/skills/ue-mcp-*`.
- **5.8 patch** (verified against `Engine/Source`):
  - *StructUtils relocation* — `InstancedStruct.h` now lives in CoreUObject: fixed the include in
    `AnimationHandlers_StateMachine.cpp`; removed the dead `StructUtils` dep from `UE_MCP_Bridge.Build.cs` and the
    plugin entry from `UE_MCP_Bridge.uplugin`.
  - *FJsonObject keys* `FString`→`UE::FSharedString` — ~16 sites across 12 handlers fixed with
    `const FString KeyStr(Pair.Key.ToView());` hoisted per `->Values` loop. (Rejected the
    `UE_JSONOBJECT_LEGACY_STRING_KEYS=1` shortcut — ABI-unsafe against a binary/installed engine.)
- **Re-wired**: `UE_MCP_Bridge` + `GameplayAbilities` (bridge hard-dep, D-6) re-enabled in `PuttBattle.uproject`;
  `npm install` restores `node_modules`; `.mcp.json` → `node node_modules/ue-mcp/dist/index.js`. The built-in
  `ModelContextProtocol` plugin stays enabled in-editor but is no longer the active MCP.
- **Docs**: D-16; `CLAUDE.md`/`docs/CONVENTIONS.md` §11, `docs/MCP-SETUP.md`, `docs/USER-ACTIONS.md` back to ue-mcp.

## Verification evidence
- **`PuttBattleEditor Win64 Development` builds clean on 5.8** — `Result: Succeeded`,
  `UnrealEditor-UE_MCP_Bridge.dll` linked, exit code 0. Only **C4996 deprecation warnings** remain in vendored
  anim/gameplay/landscape/material handlers (tolerated, like the pre-existing notices noted in D-14).
- Runtime MCP connection is the remaining human step (editor + session restart) — see below.

## Remaining (human)
1. **Reopen the UE 5.8 editor** (loads the freshly-built bridge; Output Log shows the WebSocket server on `:9877`).
2. **Restart the Claude Code session** → `/mcp` shows `ue-mcp` connected → verify with `project get_status`.
3. **License** — ue-mcp's GitHub terms suggest a commercial licence may be required for a shipping commercial game;
   verify before release.
4. The never-firing `mcp__ue-mcp__editor` hook in `.claude/settings.json` (D-15) — keep or remove (agents can't edit
   that file).

## Known issues / notes
- We maintain a **fork** of the bridge; an upstream 5.8 release would supersede these patches.
- Deprecation-warning cleanup (PoseSearch / NavMesh / Material APIs) is deferred — non-blocking on 5.8.
- `docs/pr/engine-58-upgrade.md` and D-14/D-15 are left as historical record; D-16 is the current state.
