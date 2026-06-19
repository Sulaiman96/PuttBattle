# MCP Setup — Claude Code ↔ Unreal Editor (native `unreal-mcp`)

This wires **one** editor MCP server into the repo so every Claude Code session can drive the Unreal editor.
Agent-facing rules: `CLAUDE.md` §11.

| Server | Plugin | What it's for |
|---|---|---|
| `unreal-mcp` | Epic's built-in **`ModelContextProtocol`** server + the `*Toolset` plugins (`EditorToolset`, `UMGToolSet`, `NiagaraToolsets`) | Everything: levels/actors, objects/properties, primitives, assets, Blueprints, materials, Niagara, UMG, PIE, viewport capture — one HTTP server |

> **History (DECISIONS D-17).** The third-party `ue-mcp` node client + vendored `UE_MCP_Bridge` C++ plugin
> (D-6 / restored in D-16) were **removed**: by mid-2026 Epic's native server, fed by the `*Toolset` plugins,
> exposes enough (Scene/Actor/Object/Primitive/Asset/Blueprint/Material/Niagara/UMG + a `ProgrammaticToolset`
> Python batcher) to do the level/asset/Blueprint work the bridge used to. The orphaned `GameplayAbilities`
> plugin (the bridge's only justification, D-6) was removed in the same pass. See D-14 → D-15 → D-16 → D-17 for the
> full back-and-forth.

## How it connects
The `ModelContextProtocol` plugin runs an **HTTP MCP server inside the open editor** at
**`http://127.0.0.1:8000/mcp`** (Editor Preferences → *Model Context Protocol*: Auto Start Server, port 8000,
URL `/mcp`, Enable Tool Search). `.mcp.json` points Claude Code at that URL. The editor must be open for tools to
work — by design; the agent drives your live editor. There is **no node process, no `npm install`, no `:9877`
WebSocket** any more.

The server is a **toolset registry**: instead of one tool per action it advertises three discovery meta-tools —
`list_toolsets`, `describe_toolset(name)`, `call_tool(tool_name, toolset_name, arguments)` — and the toolsets
(`editor_toolset.toolsets.scene.SceneTools`, `…actor.ActorTools`, `…object.ObjectTools`, `…primitive.PrimitiveTools`,
`…asset.AssetTools`, `…blueprint.BlueprintTools`, `…programmatic.ProgrammaticToolset`, `EditorToolset.EditorAppToolset`,
`UMGToolSet`, `NiagaraToolsets…`) come from the enabled `*Toolset` plugins. **`describe_toolset` before you call**
(read the input schema); never guess a tool name.

## Setup (fresh clone)
1. **Enable the plugins** (already committed in `PuttBattle.uproject`): `ModelContextProtocol`, `EditorToolset`,
   `UMGToolSet`, `NiagaraToolsets` (and `MCPClientToolset`). If a clone has them off: Edit → Plugins → search each →
   tick Enabled → Restart.
2. **Turn the server on:** Editor Preferences → *Model Context Protocol* → tick **Auto Start Server** (port `8000`,
   URL `/mcp`, **Enable Tool Search** on). It starts with the editor from then on.
3. **Open the editor.**
4. **Restart the Claude Code session** so it loads `.mcp.json` and connects.

## Committed config — `.mcp.json` at the repo root
```json
{
  "mcpServers": {
    "unreal-mcp": { "type": "http", "url": "http://127.0.0.1:8000/mcp" }
  }
}
```

## Key limitation (collision channels)
`ObjectTools.set_properties` can write a mesh component's `staticMesh` / transform / `physMaterialOverride` /
`overrideMaterials`, but **cannot** set a component's collision channel/enabled (`PB_Floor`/`PB_Wall` `objectType`,
`collisionEnabled`) — those go through `FBodyInstance` setters that a raw property import doesn't call. Author/finish
PB_Floor/PB_Wall collision in the editor Details panel, or by **re-shaping an already-channeled mesh** (change its
mesh/transform/material — the channel carries over untouched). See `docs/LEARNING.md`.

## Verification checklist (editor open)
1. In Claude Code: `/mcp` → `unreal-mcp` shows **connected**.
2. Ask the agent to call `list_toolsets` → returns the toolset list.
3. Ask for `SceneTools.get_current_level` → returns the loaded level path.
4. Ask it to place an actor then delete it → expect a **permission prompt** on the write (mutating actions aren't
   pre-approved — that's the guardrail), and the actor appears/disappears in the viewport.

## Troubleshooting
- **`/mcp` shows unreal-mcp not connected** → editor closed, or the server isn't running (Editor Preferences →
  Model Context Protocol → Auto Start Server, then restart the editor); reconnect the session once the editor is
  fully loaded.
- **A `call_tool` fails with "Unknown tool"** → you passed the full dotted path as `tool_name`; pass the **bare**
  method name plus `toolset_name` (the full dotted toolset path).
- **Big tool results truncate** (e.g. `CaptureViewport`, `list_properties`) → the harness saves them to a file; read
  that file (or parse server-side inside `ProgrammaticToolset.execute_tool_script` and return only what you need).
- **Requests hang ~60 s** → keep the project **out of OneDrive/Dropbox** (cloud-sync stalls file watches).

## Security posture (read once)
This server lets an AI modify your project and run editor tool-scripts. Guardrails, in order: project files are in
Git (any session's damage is a `git checkout` away — commit before big agent sessions); mutating tools are **not**
pre-approved (`.claude/settings.json` ships an empty allow-list — you approve per-use, or add specific read-only
tools yourself); the server only listens on `localhost`. Agent-facing rules: `CLAUDE.md` §11. Editing the
allow-list is a **human decision** (agents must not touch it).
