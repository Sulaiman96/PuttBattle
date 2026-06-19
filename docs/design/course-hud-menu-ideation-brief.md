# PUTT BATTLE — Design Ideation Brief

> **How to use:** copy everything below the line into a fresh Claude (claude.ai) "design" session.
> It is fully self-contained — that session has no access to this repo. It asks for course
> layouts (9 holes × 3–5 variations), a numbered cup-flag, a gameplay HUD, a leaderboard, and a
> menu — and deliberately leaves theme/art/palette for the design session to invent.

---

# PUTT BATTLE — Design Ideation Brief

You are a game / level / UI designer. Your job: invent a **world** for **Putt Battle** — a 2–6 player online mini-golf *battle* game — and produce a complete, original visual-and-layout design proposal for it.

The engine is built. The physics feel great. The chaos systems all work. **What the game does not yet have is a place to happen — that's you.** This brief hands you the machine and its rules. It tells you exactly what the game *is* and what it can *do*, and asks you to invent what it *looks and feels like*.

**Read this carefully:** every *mechanical* constraint below is load-bearing truth — design within it, don't fight it or invent new systems. But the **theme, setting, world, palette, mood, and art style are deliberately NOT specified. Inventing them is the whole point of the exercise.** Beyond "low-poly, funky, chaotic party energy," the aesthetic is yours. Pick something with teeth and commit.

---

## 1. The Fantasy (read this first, twice)

Picture six little balls — no golfers, no avatars, **you literally ARE the ball** — dropped onto a tiny low-poly diorama floating in space like a toy someone left out. A whistle blows and **all six of you putt at once.** Not turn-based. Not polite. Everyone races the same hole simultaneously as friendly ghosts (you pass *through* each other — no bumping), scrambling to sink first while **lobbing tornados, ice ages, and ink bombs at each other.**

Someone balloons your ball to triple size so it physically *can't fit in the cup*. Someone drops a bumper in the one gap you needed. The timer was a comfy 90 seconds — until the leader sank. Now a klaxon screams **"HURRY UP — 0:20"**, the music kicks up a gear, and four balls panic-putt toward the hole while a ramp launches some poor soul clean off the edge of the world. The winner does a little victory spin on a podium.

That's the feeling: **a chaotic toybox party.** Cheerful, weird, a little mean, endlessly re-playable. Chunky shapes, saturated colors, satisfying *thwacks* and *boings*. Now go build the toybox.

---

## 2. What you must deliver (exact quantities)

1. **A theme / world bible** — your invented setting, mood, and palette direction, tying holes, powerups, and UI together.
2. **9 holes**, each with **at least 3 and up to 5 distinct variations** — a minimum of **27 and up to 45 total layouts.** Every hole *and* every variation must feel genuinely unique (not a recolor).
3. **A numbered cup-flag design** (see §6 — a hard, prominent requirement).
4. **A gameplay HUD** (in-match heads-up display).
5. **A leaderboard / scoreboard** (live + end-of-hole + end-of-match states).
6. **A menu / front-end screen** (plus lobby and customiser feel).

Deliver written design docs an artist and a level builder could execute without guessing — layout descriptions, labelled/ASCII diagrams where helpful, concrete per-element specs. Name things. Be opinionated.

---

## 3. The game + what currently exists (your constraints)

These are real, already-built systems. **Design content and screens for an existing machine — do not redesign the machine, and do not invent new mechanics, surfaces, hazards, or powerups.**

- **You are the ball.** No avatar — a small sphere (~6 cm radius, centimeters-scale world), cosmetically skinnable (material / trail / hat) but mechanically just a ball.
- **The look is low-poly diorama.** Each hole is a small themed tabletop world you look down into.
- **The camera, clearly:** the in-game LOOK is **isometric / top-down diorama** (a compressed, almost-orthographic ~50–55° angle). In play, the player controls **three third-person follow presets on their own ball — close / mid / top-down — switched with keys 1/2/3.** (Note: powerup slots are *also* keys 1/2/3 — that's correct; the engine maps both, context decides.) Separately, each hole carries **2–4 fixed isometric camera vantage points** used for the hole intro fly-over and for spectators. **Design every hole to read cleanly from that compressed iso angle**, and note 2–4 fixed vantage points that frame the whole hole + its flag.
- **Simultaneous play, ghost balls.** All 2–6 players play the **same hole at the same time** and **pass through each other** — there is **no ball-to-ball physics.** The *only* way players interact is **powerups.** A hole is one shared arena 6 balls carom through at once, not separate lanes.
- **Ball rolls to rest, then shoots again.** Stroke-by-stroke, like real mini-golf. Fewer strokes is better.
- **A match is 3–9 holes** (the host picks how many). The course is assembled **randomly** from each hole's variant pool — so no two matches repeat. *This is exactly why you need many strong, distinct variants.*

---

## 4. The shot model (this dictates geometry)

- **Mouse drag to aim + set power, release to shoot.** Like a slingshot.
- The impulse is **FLAT — purely horizontal, in the ground plane.** A player can NEVER add loft or height with a shot. **All vertical motion comes from the terrain.** Ramps, lips, and slopes are the *only* way a ball goes up or gets airborne.
- So **ramps are a core design tool**: a ramp can launch a ball over a wall, across a gap, or clean **off the edge of the map** at max power (intended and valid — falling off just resets you, no penalty).
- The **ball must come fully to rest before the next shot.** Slow, long-settling surfaces (sand, sticky) cost real time — and time is what kills you when someone sinks first.

**The collision law (shapes every piece of geometry you place):** every surface is exactly one of two kinds —
- **FLOOR** — anything a ball can roll on or come to rest on (greens, ramps, paths, sand, ice…).
- **WALL** — anything a ball bounces off but can *never* rest on (rails, bumpers, borders, obstacles).

There is no third option. When you describe geometry, classify it implicitly: a rolling surface (floor) or a bouncing barrier (wall). One powerup (**Ghost Ball**) passes through *walls only* (floors still stop it) — so walls-vs-floors is a real gameplay distinction. Every hole needs **clean course bounds** (so a ghost ball has a valid place to come to rest) and a **kill-floor** beneath it to catch launched balls.

**The cup / sink rule:** the cup is a catch-zone. A ball sinks when it overlaps the cup **AND is moving slowly enough to drop** **AND isn't too big** (the Balloon powerup inflates a ball past the point the cup will accept). Fast balls lip out; settled balls drop. Design cup approaches knowing a screaming ball will skim over.

---

## 5. The mechanical toolkit (your palette — invent content, not systems)

### Surfaces (6) — the floor is a system, and a binding color language
A new player must read every surface **at a glance from the iso camera.** These colors are a **readability LAW** — design your *style* around them, never change the hues:

| Surface | Color (fixed) | Feel | Relative roll |
|---|---|---|---|
| **Fairway** | green | baseline normal roll | 1.0× (reference) |
| **Ice** | pale blue | very slidey, low drag, extra bounce | ~4× — overshoots wildly |
| **Sand** | yellow | heavy bog, kills momentum fast | ~0.3× |
| **Sticky** | purple | near-instant stop, dead (no) bounce | ~0.15× |
| **Boost** | directional **chevrons** (pattern, not just color) | accelerates the ball along an arrow while overlapping | — |
| **Mud** | (your call, distinct) | asset-defined slowdown | between sand & fairway |

Surfaces are your **personality dial:** an ice corridor that demands soft taps and bank shots; a sand belt that bogs anyone who strays; a boost-chevron speed lane that rockets a ball at a jump; a sticky pad as a safe mid-hole parking spot. Mixing surface bands is how two variants of one footprint play like different games.

### Hazards & checkpoints (your risk/reward levers)
- **Hazards** are volumes tagged **Water** or **Void.** Touching one — or **falling off the map** via the kill-floor — **respawns the ball at its last checkpoint, zero velocity, NO stroke penalty.** Falling off is lost time/position, not death.
- **Checkpoints** are placed markers with an ascending index along the intended path. You respawn at your **highest reached** checkpoint; if you've reached none, you go back to the **tee.**
- **The lever is where you DON'T place a checkpoint.** A daring shortcut over water with *no* checkpoint behind it throws a failed attempt all the way back — pure risk. A safe route studded with checkpoints costs almost nothing to fail. Build both into most holes so the clock collapse forces a gamble.

### Simultaneous timed play & the first-sink clamp (design around this)
- Default **~90-second** map timer counts down. **The instant the FIRST player sinks, remaining time clamps to ~20s** — a one-time **"HURRY UP" banner + klaxon + music-intensity** moment, fired **once** per hole.
- **DNF = 0 points** (never negative); a player at rest when the clock hits zero is DNF. Score rewards **fewer strokes, earlier sink-order, and time left on the clock.**
- A one-stroke sink is an **ace**; **consecutive aces build a visible flame multiplier** (caps ×5) shown to everyone — a brag mechanic.
- **Every hole therefore needs a fast greedy "hero line" and a safe slow line**, because the first sinker punishes everyone who played it safe.

### The 17 powerups — the chaos engine (collected from pickup boxes you place)
Players grab powerups from **bobbing pickup boxes placed in the level** (ball rolls over one → weighted random draw → granted). Inventory is **3 slots, FIFO**, persists across holes; a **full inventory refuses the draw and the box stays**; collected boxes **respawn ~10s** later. **Box count and placement are level design** — the plans define the actor's behavior, not a fixed number. Boxes on the hero line reward risk; boxes behind sand tax greed; boxes in the open bait chaos.

- **Self** (you only): **Mulligan** (redo your last shot), **Anchor** (next wall-hit dead-stops you), **Ghost Ball** (next shot passes through *walls* — not floors — must rest on valid floor in-bounds or it dies), **Teleport** (relocate your ball to a chosen nearby floor point), **Shield** (blocks the next incoming effect).
- **All-Others** (every other player): **Jumble** (hides their aim line), **Balloon** (everyone else swells ×1.6, floaty, weak shots, *too big to fit the cup*, 12s), **Reverse** (their shot goes the wrong way), **Shuffle** (randomly swaps all other balls' positions).
- **Single-Target** (one chosen enemy): **Swap** (trade places), **Lock** (they can't shoot, 6s), **Ink** (ink splats their screen, 8s), **Glue** (their next shot is weak).
- **Environment** (affects the caster too, ignores shields — **big space-reshapers, design around these**):
  - **Gust** (6s) — an **arena-wide** directional wind; bends every rolling path and nudges resting balls. *Open, exposed holes get bent hard; sheltered/walled routes resist it.*
  - **Tornado** (~15s) — a spawned actor that **patrols a straight axis**, flinging balls (with slight vertical lift). *Needs straight open room to roam; dropped in a narrow choke it can wall off the only path.*
  - **Ice Age** (10s) — the **whole course** turns to Ice. *Long open slopes and near-cup flats become overshoot chaos; walls/borders suddenly matter much more.*
  - **Bumper** (2 hits, then breaks) — a placed bounce pad that ricochets any ball hard. *Trivial in the open; a lane-plugging menace in a corridor or near the cup.*

**The key spatial dependency:** **open arenas amplify Tornado / Gust / Ice Age (high-swing crowd chaos); tight corridors amplify Bumper, Anchor, and Ghost-Ball routing plays (precise, blocking-based).** Span this spectrum across the 9 holes — some wide chaos bowls, some tight technical mazes — and individual variants can flip a hole from open to tight. **Ghost Ball rewards thick walls / wall-mazes worth cutting through — give it bait.**

---

## 6. THE NUMBERED FLAG — a hard, prominent requirement

**Every hole MUST have a large, unmissable FLAG flying high above the cup, displaying that hole's NUMBER.** This is a top-priority deliverable, not decoration:

- It is the player's **navigation beacon.** With 6 ghost balls scattered across a shared diorama and a tight clock, players must be able to **instantly locate their target cup** from the iso camera, from anywhere in the hole, every second of the round.
- It must be **tall enough to be visible over walls, ramps, and props**, and read clearly at the compressed iso FOV.
- The **hole number must be legible on the flag itself** (a big numeral — "3", "7" — on the flag/pennant/banner).
- It should fit your invented theme (a flag, a banner, a hologram, a balloon-sign, a beacon — your call on *form*) but the **function is fixed: tall, central over the cup, shows the number, always visible.**
- **Currently the game has NO in-world cup marker at all** — your flag fills a real gap and can be a **signature piece of the game's identity.** Make it iconic.
- Spec it **explicitly for every variant** — the cup placement changes per variant, so the flag moves with it; confirm it's visible from all camera vantages.

---

## 7. What to specify PER HOLE and PER VARIATION

For the **9 holes overall:** give each a one-line **identity** (its signature idea / where it sits in the round's difficulty arc) and a **par.** Holes should have nine different personalities.

**The footprint rule (what a "variant" technically IS):** all variants of one hole **share the same outer footprint / bounding box** — only the *internals* change (surfaces, walls, ramps, hazards, checkpoints, cup placement, pickup placement, camera framing). Design a hole as **"one plot of land, N different builds on it."**

For **each variation**, specify:

1. **Layout & flow** — tee position(s), the path(s) to the cup, where the cup + numbered flag sit, the overall shape (corridor / bowl / fork / spiral / maze).
2. **Hero line vs safe line** — the greedy fast route and what it risks (a jump? water with no checkpoint? an ice overshoot?) versus the slower checkpoint-protected route.
3. **Surface plan** — which surface bands go where (using the fixed color language) and *why* ("ice elbow forces a bank shot; sand apron punishes a fat hit").
4. **Hazards & checkpoints** — water/void placement, where a ramp can launch you off the map, and **deliberately where checkpoints are and aren't.**
5. **Ramps / verticality** — every up/down is authored: jumps, drops, launch-off-map edges, over-wall routes (shots are flat — terrain does all the lifting).
6. **Pickup-box placement** — how many, where, and the intent (reward risk / bait chaos / tax greed).
7. **Open-vs-tight character** — is this variant a Tornado/Gust/Ice-Age chaos arena or a Bumper/Ghost-Ball technical piece? Make it intentional.
8. **Camera framing** — 2–4 fixed iso vantage points that keep the whole hole + the numbered flag readable (also used for the intro fly-over and spectators).
9. **The numbered flag** — confirm placement and visibility from all vantages.

**What makes a variation feel UNIQUE (push hard):** change the *verb* of the hole, not the paint. Variant A is a precision putting green; B turns the same plot into an ice luge; C adds a launch ramp and a void gap so it's a jump hole; D is a tight Ghost-Ball wall-maze; E is an open Tornado bowl. Different dominant surface, route shape, risk profile, powerup-chaos flavor, and cup location. **If you can't say in one sentence how a variant plays differently, it isn't done.**

**Coverage target across the full set** — make sure the 27–45 layouts include, somewhere: at least one **ramp-jump / launch-off-map** hole, one **Ghost-Ball-bait wall maze**, one **checkpoint-gated long hole with a risky un-checkpointed shortcut**, one **open putting-duel green** (Tornado/Gust playground), at least one **ice-corridor** hole, and one hole that becomes glorious chaos under **Ice Age.**

---

## 8. The GAMEPLAY HUD

The HUD reads **replicated live game state** during a hole (it never computes truth — it displays it). It must stay readable in a chaotic 6-ball scene and **never obscure the aim/power drag.** Design the layout, hierarchy, and feel of all of these:

- **Map timer** — big countdown from ~90s; must **visibly snap/collapse** to ~0:20 when the first player sinks. Design that **"HURRY UP" banner** + timer crunch (fires once per hole).
- **Current hole # + par.**
- **Stroke count** (your strokes this hole).
- **Powerup inventory** — **3 slots, keys 1/2/3**, with icons; slots **grey out + tooltip** when unusable (e.g. finished players may use only Environment powerups).
- **Live mini-leaderboard / standings** — who's where; who has sunk and in what order (1st/2nd/3rd…).
- **Active-effect indicators on your ball** — Shielded / Ballooned / Jumbled / Reversed / Locked / Inked / Glued, etc. (presentation only).
- **Aim / power preview** — the drag indicator. Must read as **locked/disabled** during Countdown, ShotGrace, and any no-shoot window. Note: **Jumble hides it**, **Reverse inverts it** — design those states.
- **Ace-streak flame counter** — a flame / ×N multiplier shown to **everyone** while a streak is alive (loud, escalating — it's a brag).
- **Checkpoint-reached cue.**
- **Spectator strip** — small **eye-icon + name** for each finished player currently *watching you*; they animate in/out as watchers switch targets (~1s cadence). A "you're on stage" feeling.

**Phase states the HUD passes through (each a distinct screen treatment):** **HoleIntro** (~4s cinematic glance over the hole; show hole # + par; minimal HUD) → **Countdown** (big 3-2-1; shots locked) → **Playing** (full HUD) → **ShotGrace** (timer hit zero but balls still rolling from legal shots; input locked, balls finish settling) → **HoleResults** → … → **MatchResults.**

---

## 9. The LEADERBOARD / SCOREBOARD — three states

The scoring inputs (you design the *presentation*; **treat the numbers as DEFAULTS / tuning values, not fixed UI constants**):
- **StrokeScore** (points vs par, ~100 base), **PositionBonus** (by sink order, e.g. 60 / 40 / 25 / 15 / 10 / 5 for 1st–6th), **TimeBonus** (more time left = more points; **shot-grace sinks get 0**), **AceStreakBonus** (hole-in-one × consecutive-ace multiplier, caps ×5). **DNF = 0, never negative, and resets the streak.**
- **Tie-break order:** total points → fewer total strokes → lower cumulative finish time.

Design **all three presentations:**
1. **Live (in-hole):** compact running standings + a fuller **Tab scoreboard** (running totals + per-hole columns). Show sink order as it happens and any live ace-streak flames.
2. **Hole-results (~8s between holes):** per-player breakdown **rows** itemizing the components (Stroke / Position / Time / Ace-streak / DNF) — where the math is celebrated. Make a DNF sting but not cruelly; make an ace streak feel triumphant.
3. **Match-results / podium:** top-3 staging + full results table + return-to-lobby. The big finish — design the celebration.

Keep finish-order and streaks **visible and braggy** throughout — that visibility is an intended feature, not clutter.

---

## 10. The MENU / front-end

The **MainMenu is a live scene, not a flat menu** — the established direction is literally **"a ball sitting on a diorama green,"** reinforcing the tabletop identity. Design:

- The **front-end scene** (diorama framing, the ball, the mood) and how menu actions sit in / over it.
- **Primary actions:** **Create Room**, **Browse / Find Games**, **Friend Invite / Join via Steam**, **Customiser**, **Settings** (audio / video / keybinds).
- A **Lobby** feel: seats for **2–6 players**, **ready-up** per player, a **host-only settings panel** (hole count 3–9, map time, first-finish clamp, and **powerup-weight sliders** that bias the random drops). Join-in-lobby only.
- A **Customiser** with a **live ball preview** — cosmetic slots are **Material / Trail / Hat** (v1: ~10 materials, 4 trails, 4 hats; the **hat comically scales up with the Balloon powerup**). Everything is **earned** (chips or achievements) — **no microtransactions, ever** — so the screen should feel like a reward locker, not a store.

Establish the **UI skin** here: the target is a coherent **"chunky-playful"** language across every screen (mouse-first). Define typography, component shapes, button feel, and how your theme / the numbered-flag motif echo through the UI.

---

## 11. Tone, style & what's YOURS to invent

**Fixed — don't fight these:**
- Low-poly diorama, isometric look, "chunky-playful" UI, **funky / weird / chaotic party energy.**
- The **surface color language** (fairway-green / ice-pale-blue / sand-yellow / sticky-purple / boost-chevrons / water-blue) — a readability law.
- The **numbered flag** over every cup.
- The mechanical toolkit (6 surfaces, 2 hazards, 17 powerups, flat shots, ghost balls, checkpoints, the timer/clamp) — invent **content**, not systems.
- No microtransactions — cosmetics are earned.

**Yours to invent — go wild, this is the point:**
- **The theme / world / setting.** Haunted toy factory? Floating breakfast islands? Neon arcade? Garden-gnome civil war? Candy factory at dusk? Pick something with personality and make all 9 holes live in it as themed dioramas.
- The **master palette, lighting mood, and overall art style** (beyond the fixed surface colors).
- The **form of the numbered flag** (banner / pennant / hologram / beacon / balloon — as long as it's tall, central, numbered, always visible).
- The **personality of each hole and variant** within your world (a hole can be a place — "The Sticky Kitchen," "Glacier Run," "The Windmill Gauntlet").
- The **powerup visual identity**, the **UI typography / iconography**, the **menu diorama**, and the **comedy beats** (Balloon-swollen balls that can't fit the cup, hats inflating, ink on the screen, tornado yeets).

**Make it weird, make it funky, make it readable, make it fair.** Constraints are mechanical; aesthetics are yours.

---

## 12. Format your response like this

Structure the deliverable so pieces could go straight to artists and level designers:

1. **The World** — your invented theme/setting, mood, palette direction, and why it fits "low-poly funky chaos."
2. **The Flag** — what the always-present numbered cup-flag looks like in your world (a headline element — sell it).
3. **The 9 Holes** — for each: name, identity, one-line hook, par; then its **3–5 variations**, each fully specified per the §7 nine-item template. Confirm the §7 archetype coverage somewhere across the set. (This is the bulk of the work — be concrete enough to block out from your text alone.)
4. **The HUD** — annotated layout-in-words covering every §8 element + the phase states + the "HURRY UP" moment.
5. **The Leaderboard** — all three contexts (live / Tab, hole-results breakdown, podium).
6. **The Menu** — the live-scene front-end + Lobby + Customiser, in a coherent chunky-playful skin.
7. **The Through-Line** — a short closing note on how flag, HUD, leaderboard, and menu all speak the same visual language as your world.

Be evocative *and* concrete. Name the world. Name the holes. Give colors, shapes, props, signage. Make me *see* it — and where I left a call open (the theme, palette, props, signage, the flag's form, music, mascots), **commit to it confidently** and keep it coherent across everything. Now go invent somewhere weird and wonderful. The chaos is the point.
