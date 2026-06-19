# PUTT BATTLE — User Actions (the human's list)

Everything not on this list belongs to an agent. You never write code, edit configs/inis, build Blueprints, lay out maps, or write tests. Your actions fall into six unavoidable buckets: **accounts & money · GUI-only clicks · secrets · second machine/network · other humans · feel judgment**. Items are numbered UA-1…24 and referenced from the plan files.

Written for someone who has **never opened Unreal Engine**. When an agent pauses for one of these, it must walk you through with exact menu paths and verification steps (CONVENTIONS §12) — if it ever assumes knowledge you don't have, tell it to expand; that's a convention violation, not a favour.

---

## Phase 0 — Foundation (~2 h, mostly downloads)
- [ ] **UA-1 · Install the toolchain** (before T0.1): UE 5.7 (install Epic Games Launcher → Unreal Engine tab → Library → ➕ → 5.7) · Visual Studio 2022 Community — in the installer's **Workloads** tab tick *Game development with C++* and *.NET desktop development* · Git, then run `git lfs install` once in any terminal · Node.js 18+ LTS · Claude Code. *Why you: installers, licences, elevation prompts.*
- [ ] **UA-2 · Create the blank project** (5 min): Launcher → Library → Launch 5.7 → Project Browser opens → **Games → Blank**. In the right panel: switch the toggle from Blueprint to **C++**, **untick Starter Content**, raytracing off; pick your repo folder as the location, name it `PuttBattle`, Create. First launch then compiles shaders for several minutes — a progress bar bottom-right; that's normal, let it finish. *Why you: the wizard is more reliable than a hand-scaffold; one-time.* The agent verifies and configures everything after.
- [ ] **UA-3 · (Optional) Create the private GitHub repo** and authenticate `gh` so agents can push. *Why you: account auth.*

### T0.3 — MCP integration (~3 min) — native **`unreal-mcp`** (DECISIONS D-17)
> The editor MCP is now Epic's **built-in** `ModelContextProtocol` server (HTTP `:8000`), fed by the `*Toolset`
> plugins (`EditorToolset`, `UMGToolSet`, `NiagaraToolsets`). The third-party `ue-mcp` node client + `UE_MCP_Bridge`
> plugin (and the orphaned `GameplayAbilities`) were **removed** (D-17) — the native server is capable enough now.
> No `npm install`, no bridge build, no `:9877`. The MCP plugins are committed-enabled in `PuttBattle.uproject`.
> Setup detail: `docs/MCP-SETUP.md`. Old ue-mcp/VibeUE steps stay struck through for history.
- [ ] **UA-6 · Turn the server on (once).** Editor Preferences → *Model Context Protocol* → tick **Auto Start Server** (port `8000`, URL `/mcp`, **Enable Tool Search** on). *Why you: an editor GUI toggle; it then starts with the editor every time.*
- [ ] **UA-7 · Open the UE 5.8 editor**, then **restart the Claude Code session** so it reloads `.mcp.json`, and run `/mcp` → confirm **`unreal-mcp` connected**. *Why you: editor GUI + the session must reload config; this connection is the go/no-go.*
- [ ] **UA-8 · Watch a verification call** (3 min): ask the agent to call **`list_toolsets`**, then **`SceneTools.get_current_level`**; confirm a permission prompt appears on the first editor-*mutating* action (e.g. place then delete an actor). *Why you: you're testing the guardrails that protect you from agents.*
- <s>UA-6/UA-7 (ue-mcp) · `npm install` ue-mcp · load `UE_MCP_Bridge` on `:9877` · UA-4 · `npx ue-mcp init` wizard · UA-5 · decline GAS in the wizard</s> — superseded by **D-17** (ue-mcp removed; native server only).
- <s>UA-4 (older) · VibeUE API key · UA-5 · install key + `VIBEUE_API_KEY` · UA-6 · enable Unreal Claude + VibeUE plugins / auto-approve OFF · UA-7 · approve two servers</s> — superseded first by ue-mcp, then by the native server.

## Phase 1 — Ball & shot
- [ ] **UA-9 · Feel sign-off** (15 min): play the graybox hole and answer three questions: (1) does the ball slow like it has weight, or like it's on glass / in syrup? (2) can you reliably make a gentle short putt? (3) does full power feel *powerful*? Test on both mouse and trackpad. Useful vocabulary: floaty, heavy, twitchy at low power, mushy, slidey. Describe sensations — the agent translates them into damping/curve changes, and will give you `pb.*` console commands (press the backtick ` key in-game to open the console) so you can tweak values live mid-session instead of round-tripping. *Why you: agents verify math, not feel. Gates phase exit.*

## Phase 2 — Surfaces
- [ ] **UA-10 · Surface feel sign-off** (10 min): have the agent teleport your ball between unlabelled test strips; call out which surface you think you're on from feel alone. Ice/sand/boost/sticky should be unmistakable. *Gates phase exit.*

## Phase 3 — Multiplayer
> **Step-by-step test guide: `docs/pr/phase-03-testing.md`** — covers the no-Steam local PIE match test
> (Test A, do this first), the Steam two-instance flow (Test B = UA-12), packaging, and a per-feature
> verification checklist. The items below are the gates; the guide is the how-to.
- [ ] **UA-11 · Steam client** installed and logged in on the dev machine (dev AppId 480 needs it running). Only required for the Steam test (Test B); the local PIE match test (Test A) needs no Steam.
- [ ] **UA-12 · Two-machine Steam test** (30 min): second PC or a friend with the build, one 3-hole match over a real network. Inviting: with the game running, **Shift+Tab** opens the Steam overlay → Friends → right-click → *Invite to Game* (works on the dev AppId as long as both machines run the build). Full steps + what to verify in `docs/pr/phase-03-testing.md` (Test B). *Why you: agents can't operate a second Steam account on a second network.*
- [ ] **UA-13 · First playtest** (one evening): recruit 2 friends, play graybox matches, collect "what felt bad" notes. Agent prepares the build and a feedback sheet. *This is the project's most important quality gate — if rolling a ball isn't fun in graybox, stop and tune before Phase 4.*

## Phase 6 — Powerup balance
- [ ] **UA-14 · Balance session** (one evening, 3+ humans): full matches with all 17 powerups; mark anything that feels mandatory or useless. Agent supplies telemetry CSV + survey sheet and applies the tuning afterwards.

## Phase 7 — Course
- [ ] **UA-15 · Difficulty review** (1–2 h): play all 18 layouts once; flag unfair, boring, or confusing holes. Agent reworks flagged ones.

## Phase 9 — Progression
- [ ] **UA-16 · Steam Cloud roam check** (10 min): on machine 1, finish a match and quit. On machine 2 (same Steam account), launch and confirm your Chips balance and equipped cosmetics match. If not: Steam → Settings → Cloud → confirm sync is enabled on both, then report to the agent.

## Phase 10 — Art & audio
- [ ] **UA-17 · Art source decision + acquisition**: pick one — buy a low-poly kit (Fab/Synty-style; budget call), commission, or DIY in Blender. Drop files in `RawAssets/Kit/`. *Why you: money, licences, taste.* Agent imports, organises, and dresses every map.
- [ ] **UA-18 · Audio acquisition**: SFX pack + music (buy/commission/curate free-licensed). Drop in `RawAssets/Audio/`. *Why you: licensing is legal exposure.* Agent imports and wires everything.
- [ ] **UA-19 · Fresh-eyes readability test** (15 min): show someone who has never seen the game one dressed hole for ~5 seconds from the game camera, then ask them to point at: where you can roll, what's dangerous, what's ice. Hesitation = the colour language failed; tell the agent which surface confused them. *Why you: requires a human who isn't you.*

## Phase 11 — Ship
- [ ] **UA-20 · Steamworks partner account** — **START DURING PHASE 8**: signup, $100 app fee, identity/tax/bank verification. *Lead time is weeks; this is the #1 schedule risk you personally own.*
- [ ] **UA-21 · Store assets approval**: capsule art (commission or DIY), screenshot selection, trailer cut approval; submit page for Steam review. Agent drafts copy and captures footage candidates.
- [ ] **UA-22 · Pricing + ratings**: final price + regional matrix decision; confirm the ratings-questionnaire answers the agent drafts. *Why you: money + legal attestations.*
- [ ] **UA-23 · Beta recruiting**: 10–20 players (Discord/Reddit/friends), distribute keys, run the community channel.
- [ ] **UA-24 · Build uploads**: agent scripts SteamPipe; you run it (credentials/2FA) per upload.

---

**Total human load:** ~2 days of clicks and decisions plus 3–4 social evenings, spread across the whole project. If an agent session ever asks you to do something not on this list, it's either a new genuine human-only need (add it here, numbered) or the agent is offloading its job — push back.
