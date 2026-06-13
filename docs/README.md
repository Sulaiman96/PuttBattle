# PUTT BATTLE — Master Build Plan

**Version:** 2.0 · **Date:** 12 June 2026 · **Engine:** Unreal Engine 5.7 · **Module name:** `PuttBattle`

**Pitch:** A 2–6 player online mini-golf race. You *are* the ball. Everyone plays the same hole simultaneously as ghost balls, collecting stackable powerups to sabotage rivals or reshape the course, racing a map timer that collapses the moment the first player sinks. Courses resolve randomly from hole-variant pools so no two matches repeat. Low-poly diorama art, one cheap purchase, zero microtransactions — every cosmetic earned by playing.

**Reference DNA:** Putt Party (simultaneous ghost-ball play, powerup chaos, top-down framing) × Super Battle Golf (point scoring, timer pressure, host-configurable lobbies, surfaces, cosmetic progression).

---

## How to use this package

1. `CONVENTIONS.md` → copy to repo root as `CLAUDE.md` during T0.2. Every agent session reads it first.
2. `plans/00` → `plans/11` are executed in order. Tasks marked ∥ are parallel-safe within their phase.
3. Each task has a binary **Done when**. An agent does not move on until it passes.
4. This README is the design authority. If a plan file contradicts it, the README wins; flag the conflict.
5. `MCP-SETUP.md` + the `repo-root-files/` configs (`.mcp.json`, `.claude/settings.json`) wire Claude Code to the live Unreal editor — installed once during T0.3, used by every agent session after.
6. `USER-ACTIONS.md` is the **human's** list (UA-1…24): accounts, secrets, GUI clicks, second-machine tests, playtests, feel sign-offs. Plan files are agent-only and reference UA numbers where a human gates progress.
7. The human is a senior C#/.NET engineer and an **Unreal/game-dev novice learning through this project**: CONVENTIONS §12 governs how agents instruct and teach them; `LEARNING.md` is the running concept glossary agents maintain.

---

## 1. Locked Design Decisions

| # | Decision | Choice |
|---|----------|--------|
| D1 | Player embodiment | Player **is the ball**; no character |
| D2 | Control | Discrete shots: mouse drag = aim + power, clamped max drag (~0.28 viewport height). Ball rolls to rest before next shot |
| D3 | Shot plane | **Shots are always flat** (impulse in XY plane only). Vertical movement comes from terrain — ramps at speed can launch balls off the map |
| D4 | Camera | Top-down/isometric; 2–4 fixed rigs per hole variant, player cycles |
| D5 | Ball collision | **Ghost balls** — no ball-to-ball physics; interaction is powerups only |
| D6 | Powerup inventory | Stack up to **3**, persists across holes within a match |
| D7 | Water / out-of-bounds / fell off map | Reset to **last activated checkpoint** (tee if none). Checkpoints activate per-player on proximity. No penalty stroke |
| D8 | Map timer | Host-configurable **MapTime** (default 90 s). When the **first player sinks**, remaining time clamps to **FirstFinishClamp** (host-configurable, default 20 s) |
| D9 | Overtime | **Shot-grace only**: at expiry, any ball still moving from a shot taken before expiry keeps simulating until it rests — it can still sink and score. Players at rest at expiry are DNF |
| D10 | DNF | Scores **0 for the hole. No negative penalty** |
| D11 | Hole count | Host selects **3–9 holes** (minimum 3) from the course's 9 slots |
| D12 | Ace streaks | Back-to-back hole-in-ones earn escalating bonus, **capped at a 5-streak** |
| D13 | Spectating | Finished players free-switch between any remaining player; **spectated players see who is watching them**. Finished players may still use **Environment** powerups |
| D14 | Powerup config | Host adjusts per-powerup spawn weights pre-match via slider UI; probabilities always normalise to 1 |
| D15 | Targeting rules | `Self` / `SingleTarget` / `AllOthers` / `Environment`. **Environment affects everyone including the caster** (unless finished) and is **not blocked by Shield** |
| D16 | Lobby size | 2–6 players (architecture must not break at 8) |
| D17 | Monetisation | One-time cheap purchase, no MTX; cosmetics via achievements + earned currency ("Chips") |
| D18 | Art | Low-poly, diorama-framed, Super Battle Golf-inspired |
| D19 | Platform / hosting | Steam (Windows), Online Subsystem Steam, **listen server**, no backend |
| D20 | Hole intro | Quick camera glance over the hole → 3-2-1 countdown → control + timer start |
| D22 | No GAS | Powerups use the custom lean framework in plans/05, **not** Epic's Gameplay Ability System — GAS's prediction and attribute machinery solve problems this game doesn't have (discrete shots + ghost balls make latency benign) at a complexity cost a 17-effect party game can't justify. Revisit only if post-launch design grows toward stacking magnitude-calculated effects |
| D21 | C++/Blueprint split | **Hybrid:** C++ owns framework, replication, physics, match flow. Blueprint owns asset wiring, all feel-tuning values, and simple powerup effects (via a sanctioned BP API). Every feel constant is tweakable without recompiling, including live in PIE via `pb.*` console variables |

### Defaults chosen by Claude — veto any time (all are config values)

| # | Default |
|---|---------|
| DF1 | MapTime range 60–300 s (default 90); FirstFinishClamp range 10–60 s (default 20) |
| DF2 | Ace streak bonus = `AceBonusBase (75) × min(streak, 5)`; streak resets on any non-ace hole |
| DF3 | Pickups respawn 10 s after collection |
| DF4 | Playing fewer than 9 holes selects N random distinct slots (seeded), preserving course order |
| DF5 | Powerup weight sliders are **relative weights** with a live % readout (draw-time normalisation) rather than mutually-fighting absolute sliders |
| DF6 | Cup rejects a sink if ball speed > threshold or ball is Ballooned |
| DF7 | If every player finishes early, hole ends immediately after last sink |

---

## 2. Rules Specification

### 2.1 Shot
Drag from anywhere once your ball is at rest; aim is opposite the drag, power = drag length through a tunable ease-out curve (fine control at putting range). Clamped at `MaxDragScreenFraction = 0.28` of viewport height. Dotted preview to **first** bounce only, length ∝ power. Impulse is constructed strictly in the ground plane (D3). At-rest = `|v| < 5 cm/s for 0.5 s` or a 12 s roll timeout force-stops the ball. Cancel by dragging back into a dead-zone.

### 2.2 Checkpoints & hazards
`CheckpointActor` activates per-player on proximity (server-tracked, ordered). Water, void volumes, and falling below KillZ teleport the ball to its owner's latest checkpoint with zero velocity. Level-design lever: risky shortcuts may have no checkpoints.

### 2.3 Timer flow (per hole)
```
HoleIntro (camera glance ~4 s) → Countdown (3 s) → Playing (MapTime ticking)
  on first sink: Remaining = min(Remaining, FirstFinishClamp)
  at 0:00 → ShotGrace: balls launched before expiry simulate to rest (may sink & score);
            players at rest at expiry = DNF (0 pts)
→ HoleResults (8 s) → next hole / podium
```

### 2.4 Scoring (per hole)
```
HoleScore     = StrokeScore + PositionBonus + TimeBonus + AceStreakBonus
StrokeScore   = clamp(Par + 2 − Strokes, 0, ∞) × StrokePointValue   // dominant, default 100
PositionBonus = finish order table (60/40/25/15/10/5)
TimeBonus     = (TimeRemaining / MapTime) × 30                      // 0 for ShotGrace sinks
AceStreakBonus= (Strokes == 1) ? 75 × min(consecutiveAces, 5) : 0
DNF           = 0 total for the hole
```
Match tie-break: fewer total strokes, then fastest cumulative finish time. All constants in `UPBScoringConfig`.

### 2.5 Powerups — v1 set (17)

**Self (5)** — Shield does not block Environment effects.
| Powerup | Activation | Effect |
|---|---|---|
| Mulligan | Instant | Return to pre-shot position of your last stroke; stroke refunded |
| Anchor | Instant (armed) | Ball stops dead at its next surface contact |
| Ghost Ball | Armed: next shot | Ball passes through **walls** (not floors). If it comes to rest anywhere invalid → death, respawn at checkpoint |
| Teleport | Place location (radius-limited) | Instantly relocate your ball to a valid floor point within radius R |
| Shield | Passive until consumed | Blocks the next SingleTarget or AllOthers effect on you. Never blocks Environment |

**All-others (4)**
| Powerup | Duration | Effect |
|---|---|---|
| Jumble | Next stroke | Removes aim indicator + power feedback for all others |
| Balloon | Timed 12 s | All others inflate: cannot fit the cup, shot power ×0.6, floatier |
| Reverse | Next stroke | All others' aim is inverted (ball travels along the drag, not opposite) |
| Shuffle | Instant | All other active players' ball positions swap randomly among themselves |

**Single-target (4)**
| Powerup | Duration | Effect |
|---|---|---|
| Swap | Instant | Exchange your ball's position with the target's — high risk/reward |
| Lock | Timed 6 s | Target cannot take a shot while the clock keeps running |
| Ink | Timed 8 s | Target's screen is splattered, heavily obscuring their view |
| Glue | Next stroke | Target's next shot max power ×0.4 |

**Environment (4)** — affect everyone including the caster; usable while finished; ignore Shield.
| Powerup | Activation | Effect |
|---|---|---|
| Gust | Pick direction | Global wind force on all active balls for 6 s |
| Tornado | Place location + direction | Tornado patrols back and forth along the chosen axis for ~15 s, flinging any ball it touches |
| Ice Age | Instant | Global surface override: all floors behave as ice for 10 s |
| Bumper | Place location | Spawns a bumper that violently bounces any ball; breaks after 2 hits |

Acquisition: roll through pickup boxes; weighted random draw from the host-configured weight table. 3 FIFO slots, keys 1/2/3.

### 2.6 Spectating
On finishing, a player enters spectate: cycle between active players (arrow keys) and cycle that hole's camera rigs. Active players' HUDs show a spectator strip ("👁 name") of who is watching them. Spectators' inventories stay visible; only Environment powerups are activatable.

### 2.7 Match flow
```
MainMenu → Create Room | Browse | Friend Invite
→ Lobby: ready-up · host settings (holes 3–9, MapTime, FirstFinishClamp, powerup weight sliders)
→ per hole: VariantReveal/Glance → Countdown → Playing → [ShotGrace] → HoleResults
→ MatchResults / Podium → Lobby
```

### 2.8 Progression
Chips earned from match score → spend on ball cosmetics (materials, trails, hats); other cosmetics gated by achievements (mirrored to Steam). Persistence: SaveGame + Steam Cloud. No backend, no server-side validation — accepted trade for a no-MTX game.

---

## 3. Architecture (summary — details live in plan files)

Server-authoritative everything that scores; listen server. Shot = validated server RPC; physics replication uses Predictive Interpolation for the owned ball. Ghost balls via a dedicated collision channel. All effects execute server-side and mutate a replicated `FPBBallAttributes` struct (power multiplier, scale, aim flags, shield, armed states) — **effects never set ad-hoc properties on the pawn**. Because effects run server-side inside a C++ framework, simple effects can safely be *implemented in Blueprint* (D21) — the framework owns all replication; the BP just describes what the effect does.

**The five extensibility surfaces:**
1. **Powerups** — `UPBPowerupDefinition` asset + one `UPBPowerupEffect` subclass. Four targeting rules × four activation modes (Instant / PickDirection / PlaceLocation / PlaceLocationDirection) are framework-level.
2. **Surfaces** — `UPBPhysicalMaterial` + `UPBSurfaceDefinition` asset pair, zero code. A **global override stack** supports whole-map surface effects (Ice Age).
3. **Course variation** — 9 `UPBHoleDefinition` slots × 1–6 variant Level Instances; server-seeded resolution; host picks 3–9 holes.
4. **Scoring/timers** — every constant in `UPBScoringConfig` / lobby `FPBMatchSettings`.
5. **Cosmetics/achievements** — definition assets with unlock conditions.

**Extensibility contract:** if a new powerup, surface, hole, or cosmetic requires editing core classes, the feature design is wrong — stop and rethink.

---

## 4. Phase index

| Plan file | Contents |
|---|---|
| `plans/00-foundation.md` | Project bootstrap, Git/LFS, collision channels, tags, input, CLAUDE.md |
| `plans/01-ball-and-shot.md` | Ball pawn, attributes struct, flat-shot system, graybox hole, camera rigs |
| `plans/02-surfaces-hazards-checkpoints.md` | Surface framework + override stack, surface set, hazards, checkpoints |
| `plans/03-multiplayer-core.md` | Replication, match phase skeleton, Steam sessions, lobby |
| `plans/04-match-flow-timers-scoring.md` | Full timer model, DNF, ace streaks, scoreboard, podium |
| `plans/05-powerup-framework.md` | Definitions, inventory, pickups, targeting/placement UI, weight sliders, effect runtime |
| `plans/06-powerup-effects.md` | All 17 effects, one task each |
| `plans/07-course-variation.md` | Hole/course assets, CourseDirector, streaming, variant reveal |
| `plans/08-spectator-system.md` | Spectate cycling, spectator visibility, finished-player env powerups |
| `plans/09-progression-cosmetics.md` | Chips, saves, cosmetics, achievements |
| `plans/10-art-audio-polish.md` | Low-poly kit, VFX/SFX, UI skin, juice |
| `plans/11-ship.md` | Packaging, Steamworks, playtests, perf, launch |

---

## 5. v1 Scope fence
No dedicated servers · no host migration · no crossplay (OSS abstraction keeps EOS open) · no ranked/MMR · no level editor/mods · no consoles · no backend · no anti-cheat beyond server authority · powerup count frozen at 17 until post-launch.

## 6. Risks
1. **Phase 5–6 is now the biggest code chunk** (17 effects + 4 activation modes + placement UI). Mitigation: framework first, then effects in the order listed in `06` — the first eight constitute a shippable fallback set.
2. **Content cost**: 9 slots × 2 variants = 18 layouts is the long pole. Modular kit pieces; variants share footprints.
3. **Ghost Ball depends on authoring discipline** — wall/floor channel contract is enforced by an editor validation script (see plans/07).
4. **Physics feel is the product** — first playtest end of Phase 3 with grayboxes, not Phase 10.
5. **Listen server**: host quit ends match; surfaced honestly in UX.
