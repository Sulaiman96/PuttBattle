# MCP Setup — Claude Code ↔ Unreal Editor (built-in Unreal MCP)

The editor is driven through Epic's first-party **Unreal MCP** plugin (`ModelContextProtocol`,
Experimental, shipped in UE 5.8). It exposes an HTTP+SSE MCP server that Claude Code connects to —
no third-party plugin, no npm package, no WebSocket bridge. Agent-facing rules: `CLAUDE.md` §11.

| Server | Plugin | Endpoint | What it's for |
|---|---|---|---|
| `unreal-mcp` | `ModelContextProtocol` (built-in, Experimental) | `http://127.0.0.1:8000/mcp` (HTTP+SSE) | Editor automation, exposed as *toolsets* — discover live with `list_toolsets` |

> **History.** This project previously used the third-party `ue-mcp` / `UE_MCP_Bridge` bridge (and before
> that, UnrealClaude + VibeUE). `ue-mcp` does not compile on UE 5.8, so the 5.7→5.8 upgrade switched to the
> first-party server and then **removed ue-mcp entirely** (DECISIONS **D-6, D-14, D-15**). The built-in
> server is much narrower than ue-mcp was; if you ever need that breadth back, the path is upstream shipping
> 5.8 support — re-add it fresh then.

## How it connects
The `ModelContextProtocol` plugin runs an HTTP server **inside the open editor**. `.mcp.json` (repo root)
points Claude Code at it:
```json
{
  "mcpServers": {
    "unreal-mcp": { "type": "http", "url": "http://127.0.0.1:8000/mcp" }
  }
}
```
Auto-start is configured per-project in `Config/DefaultEditorPerProjectUserSettings.ini`:
```ini
[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]
bAutoStartServer=True
ServerPortNumber=8000
ServerUrlPath=/mcp
```
The editor must be open for tools to work — by design; the agent drives your live editor. Only one editor
instance can bind `:8000`, so a second instance (e.g. the Phase-3 two-instance Steam test) runs without MCP.

## One-time setup
The `ModelContextProtocol` plugin is enabled in `PuttBattle.uproject` and the two config files above are
committed, so on a fresh clone there is **nothing to install** — the server ships with the engine. The human
does two things (UA-6, UA-7 in `docs/USER-ACTIONS.md`):

1. **Open the UE 5.8 editor once** and confirm the Output Log shows the MCP server starting on `:8000`.
   (Optional: **Edit ▸ Plugins** → search **"Unreal MCP"** → confirm it and **Toolset Registry** show
   **Enabled**.)
2. **Restart the Claude Code session** with the editor open, so it reloads `.mcp.json` and connects.

## Verification checklist (editor open)
1. `netstat -ano | findstr 8000` → the server is listening. (Or `GET http://127.0.0.1:8000/mcp` → **HTTP 405**,
   which is correct — the endpoint expects POST/SSE.)
2. In Claude Code: `/mcp` → `unreal-mcp` shows **connected**.
3. Ask the agent to call `list_toolsets` → it returns the available toolsets. Capabilities are discovered
   live; expect far fewer than the old bridge offered.

## Troubleshooting
- **`/mcp` shows unreal-mcp not connected** → editor closed / still loading, or the session started before
  the editor came up. Open the editor fully, then re-run `claude` (or `/mcp` → reconnect).
- **Nothing on `:8000`** → confirm `ModelContextProtocol` is enabled in `PuttBattle.uproject` and
  `Config/DefaultEditorPerProjectUserSettings.ini` has `bAutoStartServer=True`; check the editor Output Log.
- **A capability is missing** (no Niagara / Blueprint-graph / landscape tool, etc.) → expected; the built-in
  server is narrow. Do the work in C++ / data assets, or list it as a human editor step.
- **Requests hang** → keep the project **out of OneDrive/Dropbox** (cloud-sync stalls file watches).

## Security posture (read once)
This server lets an AI modify your project and run editor scripts. Guardrails: project files are in Git (any
session's damage is a `git checkout` away — commit before big agent sessions); the server binds **localhost**
only; mutating actions surface a per-use permission prompt. The committed `.claude/settings.json` ships an
empty allow-list — you approve per-use, or add specific read-only actions yourself. Editing that allow-list is
a **human decision**; agents must not touch it (`CLAUDE.md` §11).
