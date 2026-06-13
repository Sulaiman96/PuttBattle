# MCP Setup — Claude Code ↔ Unreal Editor

This wires two editor MCP servers into the repo so **every future Claude Code session gets Unreal editor control automatically** — no per-session configuration. Verified against both repos on 12 Jun 2026; both target **EngineVersion 5.7.0**, matching this project.

| Server | Plugin | What it's for |
|---|---|---|
| `unreal` | [Natfii/UnrealClaude](https://github.com/Natfii/UnrealClaude) v1.5 | Actors, levels, viewport capture, console commands, output log, async tasks, **UE 5.7 docs on demand** (`unreal_get_ue_context`) |
| `vibeue` | [kevinpbuckley/VibeUE](https://github.com/kevinpbuckley/VibeUE) v4.0 | Blueprints, materials, assets, Niagara, UMG, gameplay tags — 32 Python services / 1000+ methods, 35 lazy-loaded skill packs |

How it connects: each plugin runs an HTTP server **inside the open editor** (UnrealClaude on `:3000`, VibeUE on `:8088`). Claude Code talks to them via the two stdio entries in `.mcp.json` at the repo root. **The editor must be open for the tools to work** — that's by design; the agent is driving your live editor.

---

## One-time setup — task T0.3

**Division of labour:** steps 1–3 below (clone, build, npm) are **agent-executable** — let Claude Code run them as part of T0.3. Steps 4–7 are **yours**: account + API key (UA-4/5), editor GUI toggles (UA-6), first-run server approval (UA-7). Full human list: `USER-ACTIONS.md`.

**Prereqs:** UE 5.7 installed · Node.js 18+ · Git · Claude Code CLI.

### 1. Install the plugins
```bash
cd <repo>/Plugins
git clone --recurse-submodules https://github.com/Natfii/UnrealClaude.git
git clone https://github.com/kevinpbuckley/VibeUE.git
```
UnrealClaude's MCP bridge is a **submodule** — if `Plugins/UnrealClaude/UnrealClaude/Resources/mcp-bridge` is empty, run `git submodule update --init` inside it. Note UnrealClaude nests its plugin: the `.uplugin` lives at `UnrealClaude/UnrealClaude/` — move the inner folder up so the path is `Plugins/UnrealClaude/UnrealClaude.uplugin` (keep `Resources/mcp-bridge` with it), or adjust `.mcp.json` to the actual `index.js` path. **Whatever you choose, the `args` path in `.mcp.json` must point at the real `index.js`.**

**Vendoring decision (recommended):** delete the two inner `.git` folders and commit the plugins to the repo. This pins exact versions — agents can't be broken by an upstream push — and a fresh clone works immediately. Upgrading becomes a deliberate re-clone, which is what you want.

### 2. Build
Right-click `PuttBattle.uproject` → Generate project files → build the project (project-level source plugins compile with it). If UnrealClaude complains, use its standalone route from the README: `RunUAT.bat BuildPlugin -Plugin=...`. VibeUE also ships `Plugins/VibeUE/BuildPlugin.bat "C:\Program Files\Epic Games\UE_5.7"` as a fallback.

### 3. Bridge dependencies
```bash
cd Plugins/UnrealClaude/Resources/mcp-bridge
npm install
```

### 4. Editor configuration
1. Open the editor → Plugins browser → confirm **Unreal Claude** and **VibeUE** are enabled (Editor category) → restart if prompted.
2. **VibeUE API key (required):** Tools → VibeUE → AI Chat → ⚙️ gear → paste a free key from vibeue.com/login → Save. Without it, every VibeUE MCP tool errors at startup.
3. **Project Settings → Plugins → Unreal Claude → Auto-approve script execution: leave OFF.** Turn it on later only if per-script confirmation becomes genuine friction — it's the difference between the agent asking before running Python in your editor and not asking.

### 5. Environment variable (the key, never committed)
`.mcp.json` references `${VIBEUE_API_KEY}` so the secret stays out of Git. Set it once (same key as step 4):
```powershell
setx VIBEUE_API_KEY "vk_..."   # Windows — then restart your terminal
```

### 6. CLAUDE.md import
After plugins are installed, add this line near the top of the repo `CLAUDE.md` (it inlines VibeUE's agent rules — don't add it before the file exists or the import warns):
```markdown
@Plugins/VibeUE/Content/samples/AGENTS.md.sample
```

### 7. First run
Launch `claude` **from the repo root** (the `.mcp.json` paths are root-relative). Claude Code will ask you to approve the project's MCP servers — approve both. `.claude/settings.json` pre-approves the read-only tools (status, docs context, discovery, skills, logs); anything that *changes* the editor (Python execution, asset ops, actor spawn/delete) prompts per-use until you deliberately add it to the allow list.

---

## Verification checklist
With the editor open:
1. `curl http://localhost:3000/mcp/status` → JSON with project info (UnrealClaude up).
2. `netstat -ano | findstr :8088` → listening (VibeUE up).
3. In Claude Code: `/mcp` → both `unreal` and `vibeue` show **connected**.
4. Prompt: *"Call unreal_status"* → returns project/context info.
5. Prompt: *"discover_python_module('unreal', name_filter='Blueprint')"* → returns classes (proves the API key is valid).
6. Prompt: *"Spawn a cube at the origin, screenshot the viewport, then delete the cube."* → expect permission prompts for the spawn/delete (correct — they're not allowlisted), and a screenshot you can see.

## Troubleshooting
- **Tools listed but every call fails** → the editor isn't open, or the plugin didn't load (check Output Log for `LogUnrealClaude` / VibeUE startup lines).
- **VibeUE tools all error** → missing/invalid API key (editor gear icon) or `VIBEUE_API_KEY` env var not visible to the shell that launched `claude`.
- **`unreal` server won't connect** → `npm install` never ran in `mcp-bridge`, or port 3000 is taken.
- **Requests hang ~60 s** → upstream-known issue with cloud-synced folders: keep the project **out of OneDrive/Dropbox**.
- **Fresh machine** → plugins vendored: clone repo, build, steps 4–5 only.

## Fallback plan (if a plugin dies)
Both plugins are young open-source projects; abandonment or breakage on an engine update is a real risk. The designated fallback is **`ue-mcp`** (npm, `npx ue-mcp init` — db-lyon.github.io/ue-mcp): a single actively-maintained server broad enough to replace *both* plugins (levels/actors + Blueprints/materials/assets, own C++ bridge plugin). **Trigger condition:** a current plugin is broken for >1 task and unmaintained upstream — then consolidate onto ue-mcp (swap `.mcp.json`, redo the permission allowlist, update CONVENTIONS §11's division of labour). Do NOT run it alongside the current two: three overlapping servers means tool-choice ambiguity and context bloat for every agent session. If executing this plan: during `npx ue-mcp init`, select only needed tool categories and **decline any GAS-related plugin enables** (D22); vendor its deployed bridge plugin (strip `.git`, commit) like the others.

## Security posture (read once, it matters)
These tools let an AI run Python in your editor and modify your project. The guardrails, in order: project files are in Git (any session's damage is a `git checkout` away — commit before big agent sessions); destructive tools are not pre-approved; auto-approve script execution stays OFF until trust is earned; the API key lives in your environment, not the repo. Agent-facing rules live in `CLAUDE.md` §11.
