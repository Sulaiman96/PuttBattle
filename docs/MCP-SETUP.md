# MCP Setup — Claude Code ↔ Unreal Editor (ue-mcp)

This wires **one** editor MCP server into the repo so every Claude Code session gets Unreal editor
control automatically — no per-session configuration.

| Server | Package / plugin | What it's for |
|---|---|---|
| `ue-mcp` | [db-lyon/ue-mcp](https://github.com/db-lyon/ue-mcp) (npm `ue-mcp`) + `UE_MCP_Bridge` C++ plugin | Everything: levels/actors, Blueprints, materials, Niagara, assets, PIE, console — 20 category tools / 500+ actions, one server |

**Why one server (was two).** This project originally ran `unreal` (UnrealClaude) + `vibeue` (VibeUE).
VibeUE would not connect reliably (its in-editor service + API-key remote bridge kept dropping), and both
were young single-maintainer plugins. We executed the pre-agreed consolidation onto `ue-mcp` — the
fallback named in the prior version of this file — see `docs/DECISIONS.md` **D-6**. `ue-mcp` is a single
actively-maintained server broad enough to replace both. Do **not** reintroduce the old two alongside it:
overlapping servers mean tool-choice ambiguity and context bloat.

How it connects: a TypeScript MCP server (run by `npx ue-mcp …` per `.mcp.json`) talks JSON-RPC over a
**WebSocket on `ws://localhost:9877`** to the `UE_MCP_Bridge` C++ plugin running **inside the open
editor**. The editor must be open for tools to work — by design; the agent drives your live editor.
Supported: Windows UE **5.4–5.7** (we're 5.7).

---

## One-time setup — interactive (yours: UA-4…7)

The installer (`npx ue-mcp init`) is an **interactive wizard with no unattended flags**, it deploys a C++
plugin that must compile, and it offers to enable plugins — so it's a human step. The agent does the repo
plumbing (removing the old plugins, the `.mcp.json`/`.uproject`/docs edits) and the post-install vendor +
verify; you run the wizard.

**Prereqs:** UE 5.7 · Node.js 18+ · Claude Code CLI. **Close the Unreal editor first.**

### 1. Run the wizard (UA-4) — from the repo root
```bash
cd "<repo root, the folder with PuttBattle.uproject>"
npx ue-mcp init
```
It will: auto-detect `PuttBattle.uproject`; ask **which tool categories to enable**; deploy the bridge to
`Plugins/UE_MCP_Bridge/`; offer to enable UE plugins; and configure your MCP client (pick **Claude Code**).

### 2. Decline GAS (UA-5) — **critical (D22)**
When the wizard lists tool categories / plugins to enable, **do not enable GameplayAbilities / GAS** (or
any ability-system category). This project deliberately does not use GAS. Enhanced Input and Niagara are
already on; leaving the rest at the wizard's sensible defaults is fine. (The agent re-checks `.uproject`
afterward and strips `GameplayAbilities` if it slipped in.)

### 3. Compile the bridge + restart (UA-6)
Open the editor. When it asks **"compile UE_MCP_Bridge?"** click **Yes** (first compile ~30–60 s). Let it
finish loading. If prompted that the project needs rebuilding, allow it.

### 4. First run + approval (UA-7)
Launch `claude` from the repo root. Approve the project's `ue-mcp` MCP server when prompted. Per-use
permission prompts for editor-mutating actions are expected — that's the guardrail (see Security posture).

---

## Committed config
`.mcp.json` at the repo root (also mirrored in `docs/repo-root-files/`):
```json
{
  "mcpServers": {
    "ue-mcp": { "command": "npx", "args": ["ue-mcp", "PuttBattle.uproject"] }
  }
}
```
We keep the `.uproject` path **relative** so a fresh clone works on any machine (Claude is always launched
from the repo root). If `npx ue-mcp init` rewrites it to an absolute path, the agent restores the relative
form during verification.

**Vendoring:** commit `Plugins/UE_MCP_Bridge/` to the repo (strip any inner `.git`), exactly as the old
plugins were vendored — this pins the bridge version so a fresh clone + build just works. Never commit the
plugin's `Binaries/`, `Intermediate/`.

## Verification checklist (editor open)
1. `netstat -ano | findstr 9877` → the bridge is listening.
2. In Claude Code: `/mcp` → `ue-mcp` shows **connected**.
3. Prompt: *"Call ue-mcp project get_status"* → returns project/engine info.
4. Prompt: *"Get the level outliner"* (`level(action="get_outliner")`) → read works.
5. Prompt: *"Place a cube at the origin, then delete it."* → expect a **permission prompt** on the write
   (correct — mutating actions aren't pre-approved), and the cube appears/disappears in the viewport.

## Troubleshooting
- **Tools listed but every call fails** → editor not open, or `UE_MCP_Bridge` didn't load/compile (check
  Output Log for the bridge's startup lines; re-trigger the compile).
- **`/mcp` shows ue-mcp not connected** → editor closed, port 9877 taken, or `npx` can't fetch the package
  (network). Re-run `claude` after the editor is fully loaded.
- **GAS got enabled** → remove the `GameplayAbilities` plugin entry from `PuttBattle.uproject` (D22).
- **Requests hang ~60 s** → keep the project **out of OneDrive/Dropbox** (cloud-sync stalls file watches).
- **Fresh machine** → bridge is vendored: clone repo, build, then steps 3–4 only (no re-init needed unless
  changing tool categories).

## Security posture (read once)
This server lets an AI modify your project and run editor scripts. Guardrails, in order: project files are
in Git (any session's damage is a `git checkout` away — commit before big agent sessions); mutating tools
are **not** pre-approved (`.claude/settings.json` ships an empty allow-list — you approve per-use, or add
specific read-only actions yourself); the bridge only listens on `localhost`. Agent-facing rules: `CLAUDE.md` §11.
