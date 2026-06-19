# MCP Setup — Claude Code ↔ Unreal Editor (ue-mcp)

This wires **one** editor MCP server into the repo so every Claude Code session can drive the Unreal editor.
Agent-facing rules: `CLAUDE.md` §11.

| Server | Package / plugin | What it's for |
|---|---|---|
| `ue-mcp` | [db-lyon/ue-mcp](https://github.com/db-lyon/ue-mcp) (npm `ue-mcp`) + the vendored `UE_MCP_Bridge` C++ plugin | Everything: levels/actors, Blueprints, UMG, materials, Niagara, assets, PIE, console — ~20 category tools / 500+ actions, one server |

> **UE 5.8 note (DECISIONS D-16).** Upstream ue-mcp targets UE **5.4–5.7** and does **not** compile on 5.8. This
> repo carries a **vendored fork** of `UE_MCP_Bridge` **patched to build on 5.8** (the `InstancedStruct`/StructUtils
> relocation + the `FJsonObject` `FSharedString`-key change — see D-16). We briefly switched to Epic's built-in
> `ModelContextProtocol` (D-14/D-15), but it was too narrow (no Blueprint/UMG editing), so ue-mcp was restored. The
> built-in plugin stays enabled in-editor but is **not** the active MCP. If upstream ever ships 5.8 support, drop
> the fork for it.

## How it connects
A TypeScript MCP server (run by **`node node_modules/ue-mcp/dist/index.js PuttBattle.uproject`** per `.mcp.json`)
talks JSON-RPC over a **WebSocket on `ws://localhost:9877`** to the `UE_MCP_Bridge` C++ plugin running **inside the
open editor** (module loading phase `PostEngineInit`). The editor must be open for tools to work — by design; the
agent drives your live editor.

**Why `node`, not `npx`** (DECISIONS D-6): on Windows + Node ≥ 22, Claude Code cannot spawn an `npx`-based server
(`npx` resolves to a `.cmd` shim Node refuses to spawn — `ENOENT`/`EINVAL` — and the MCP handshake never completes).
Invoke **`node` on the entry script directly** — it spawns cleanly in ~250 ms. Paths are relative (Claude runs from
the repo root), so the entry is portable.

## Setup
The bridge is **vendored and committed** (`Plugins/UE_MCP_Bridge/`, Source + `.uplugin`), enabled in
`PuttBattle.uproject` (with its `GameplayAbilities` hard-dependency, D-6), and patched for 5.8 — so there is **no
`npx ue-mcp init` wizard step**. On a fresh clone:
1. **`npm install`** from the repo root — restores `node_modules/ue-mcp` (the Node server; `node_modules/` is
   git-ignored, `ue-mcp` is a `devDependency`).
2. **Build the editor target.** The bridge is an Editor C++ module, so it compiles with `PuttBattleEditor`
   (Win64 Development). **The editor must be closed to build** (it locks the editor DLL); reopen afterward.
   (`Binaries/`/`Intermediate/` are git-ignored, so a clone compiles the bridge on first build / first editor open.)
3. **Open the editor**; let `UE_MCP_Bridge` load (Output Log shows the WebSocket server on `:9877`).
4. **Restart the Claude Code session** so it loads `.mcp.json` and connects.

## Committed config — `node`-direct, not `npx`
`.mcp.json` at the repo root:
```json
{
  "mcpServers": {
    "ue-mcp": { "command": "node", "args": ["node_modules/ue-mcp/dist/index.js", "PuttBattle.uproject"] }
  }
}
```
`ue-mcp.yml` (repo root) configures the bridge: content roots and **disabled tool categories** (`pcg`, `foliage`,
`gas`, `demo`). The `gas` category is disabled so **no GAS is ever authored** (D22) even though the bridge
force-enables the `GameplayAbilities` plugin as a hard dependency — GAS stays enabled-but-unused, editor-tool-only.

## Verification checklist (editor open)
1. `netstat -ano | findstr 9877` → the bridge is listening.
2. In Claude Code: `/mcp` → `ue-mcp` shows **connected**.
3. Prompt: *"Call ue-mcp project get_status"* → returns project/engine info (UE 5.8).
4. Prompt: *"Get the level outliner"* (`level(action="get_outliner")`) → read works.
5. Prompt: *"Place a cube at the origin, then delete it."* → expect a **permission prompt** on the write (mutating
   actions aren't pre-approved — that's the guardrail), and the cube appears/disappears in the viewport.

## Troubleshooting
- **`/mcp` shows ue-mcp not connected** → editor closed / bridge not loaded; OR `node_modules/ue-mcp` is missing
  (run `npm install`); OR `.mcp.json` got reverted to an `npx` command (it must be
  `node node_modules/ue-mcp/dist/index.js`). Reconnect after the editor is fully loaded.
- **Tools listed but every call fails** → editor not open, or `UE_MCP_Bridge` didn't load/compile (check the Output
  Log for its startup lines; rebuild the editor target with the editor **closed**).
- **Bridge won't compile after an engine update** → expect the D-16 class of breaks (`FJsonObject` keys, relocated
  headers); fix against `Engine/Source` (§11 rule 9) — the bridge is a fork we maintain until upstream ships 5.8.
- **Requests hang ~60 s** → keep the project **out of OneDrive/Dropbox** (cloud-sync stalls file watches).

## Security posture (read once)
This server lets an AI modify your project and run editor scripts. Guardrails, in order: project files are in Git
(any session's damage is a `git checkout` away — commit before big agent sessions); mutating tools are **not**
pre-approved (`.claude/settings.json` ships an empty allow-list — you approve per-use, or add specific read-only
actions yourself); the bridge only listens on `localhost`. Agent-facing rules: `CLAUDE.md` §11. Editing the
allow-list is a **human decision** (agents must not touch it).
