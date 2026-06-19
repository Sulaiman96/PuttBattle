# Phase 1 & 2 — How to Test & Tune (ball, shot, surfaces, hazards, checkpoints)

A step-by-step guide for **feel-testing and tuning** everything Phase 1 (ball & shot) and Phase 2
(surfaces, hazards, checkpoints) added. Written assuming you've **never feel-tuned a game build before** —
exact menu paths, exact console commands, and "what you should see" at each step (CONVENTIONS §12).

This is the **single-player feel pass**: play the graybox hole, judge how the ball *feels*, turn the tuning
knobs live, and write down everything that feels off so we can fix it all in one batch. There's **no Steam and
no multiplayer here** — that comes later (Phase 3 packaged build, via [phase-03-testing.md](phase-03-testing.md)).

| | Needs Steam? | Needs multiplayer? | What it proves |
|---|---|---|---|
| **This guide — single-player PIE** | No | No | Ball feel, shot feel, the six surfaces, hazards & checkpoint respawn (Phase 1 + 2). The "does it feel good?" pass. |
| Phase 3 (separate doc) | Yes (UA‑11) | Yes (listen server + clients) | The match + session layer. Do **after** feel is dialed in. |

> **Heads-up about the invisible ball:** the missing-ball issue you saw is specific to the **multiplayer match
> hop** (lobby → match travel, Phase 3). **Single-player on `V_A` spawns and shows the ball normally** — which
> is exactly why all feel-testing happens here, not through the lobby.

---

## What Phase 1 & 2 added (so you know what you're looking at)

- **A physics ball** — a real rolling sphere (Chaos physics): it accelerates, rolls, loses speed, and settles.
  Mass, damping, top speed and shot power are all tunable live.
- **Drag-to-shoot** — press-and-drag the mouse to aim and set power (like a slingshot: drag *away* from where
  you want to go), release to fire. A dead-zone stops accidental taps; you can only shoot once the ball is at rest.
- **Six surfaces** — Fairway, Ice, Sand, Sticky, Boost, Mud. Each changes how far/fast the ball rolls. Adding a
  surface is data-only (no code), so tuning one is just editing an asset.
- **Hazards & checkpoints** — roll into water or off the edge of the world and you respawn at the **last
  checkpoint you passed** (or the tee if you haven't passed one), with no stroke penalty.
- **Three camera angles** — switch between zoomed-in / mid / top-down follow cameras with **1 / 2 / 3**.
- **Live tuning knobs** — a set of `pb.*` console commands let you change feel values *while playing*, no
  recompile. This guide leans on them heavily.

---

## Prerequisites

1. **Build the latest code (only if you pulled new C++).** If `Source/` changed since your last build: close the
   editor, build the **`PuttBattleEditor Win64 Development`** target, reopen. (If nothing in `Source/` changed,
   skip this — the current editor build is fine.)
2. **Set Play-in-Editor to single player.** You set this to *Listen Server / 3 players* for the Phase 3 test —
   **change it back** or you'll be feel-testing through network latency:
   - Click the **dropdown arrow next to the green ▶ Play button** on the main toolbar.
   - Set **Number of Players = 1**.
   - Set **Net Mode = Play Standalone**.
   - If those aren't in the dropdown: **Edit → Editor Preferences → (Level Editor) Play → Multiplayer Options**,
     set **Number of Players = 1** and **Net Mode = Play Standalone** there.
   - *Why:* Phase 1/2 is offline feel. A listen server adds replication smoothing/latency that hides base-feel bugs.
3. **Open the test hole.** **File → Open Level → `Content/Maps/Holes/H_Test/V_A`** (or double-click `V_A` in the
   Content Browser at `Content/Maps/Holes/H_Test/`).

### The console is your tuning panel

Almost every feel value is tunable **live, while playing**, from the in-game console:

- Press **Play (▶)**, then press the **`` ` ``** (backtick, top-left under Esc) key to open the console.
- Type a command and press **Enter**. Example: `pb.Ball.LinearDamping 0.2` makes the ball stickier *immediately*.
- **The `-1` rule:** every tuning CVar defaults to **`-1`**, which means *"use the value authored on the
  Blueprint."* Set it to **any value ≥ 0 to override live**; set it back to **`-1`** to return to the authored
  value. (So `0` is a real override meaning "zero" — e.g. `pb.Ball.LinearDamping 0` = no slowdown at all.)
- `pb.Ball.Dump` prints the ball's current effective values — use it to confirm what's actually in effect.

---

## The map at a glance (`V_A`)

`V_A` is one graybox lane built to exercise everything at once:

- A **tee** at one end (where you spawn / restart), a **cup** (hole) on the Fairway band.
- **Six surface bands** laid side by side along the lane: **Mud · Sticky · Fairway (with the cup) · Ice · Sand · Boost**.
- **Two checkpoints** along the fairway lane (index **0** then index **1**).
- A **water hazard** over the sand band.
- A **ramp** at the far end (for testing balls launching off the map).
- **KillZ = −500** (the invisible floor: fall below it and you respawn).

Everything is untextured graybox — art and sound are Phase 10. You're judging *motion and feel*, not looks.

---

## Controls

| Input | Does |
|---|---|
| **LMB press + drag** | Aim & set power. Drag **away** from your target (slingshot). Drag length = power; a longer drag = more power. |
| **LMB release** | Fire the shot (unless the drag was inside the dead-zone, which cancels). |
| **RMB** or **Esc** | Cancel the current aim (no shot fired). |
| **R** | Restart the hole (ball back to tee, strokes reset). |
| **1 / 2 / 3** | Switch camera angle: **1** zoomed-in, **2** mid (default), **3** top-down. |
| **`` ` ``** (backtick) | Open/close the console for `pb.*` tuning commands. |

> The old **C** "cycle camera" key was retired (decision D‑7) — camera is now the 1/2/3 presets. Pressing C does
> nothing on `V_A`.

---

## Test 1 — Ball & shot feel (Phase 1)

Press **Play**, then work through these on the **Fairway band** (the middle one, with the cup). Tick each box.

- [ ] **Spawn & rest.** The ball sits on the fairway, not sinking into or floating above it. It's not drifting.
- [ ] **Gentle putt.** Drag a short distance and release. The ball rolls a little, slows **smoothly**, and comes
      to a complete stop. *Bad feel to note:* slides forever (too little damping) / stops dead like glue (too much).
- [ ] **Full-power putt.** Drag about a quarter of the screen height and release. The ball takes off fast, rolls
      a long way, then settles. *Judge:* does it feel **powerful and weighty**, or floaty/weak?
- [ ] **Dead-zone.** Click and release with almost no drag (a tiny twitch). **No shot fires** (it's treated as a
      mis-click). A slightly longer drag *does* fire a weak putt.
- [ ] **At-rest gate.** Fire a shot, then immediately try to aim again while the ball is still rolling. You
      **can't** — aiming only unlocks once the ball is nearly stopped (about half a second after it settles).
- [ ] **Restart.** Press **R**. Ball returns to the tee, stroke counter resets.
- [ ] **Camera.** Press **1 / 2 / 3** mid-aim. The view changes; the aim arrow re-orients to the new camera so
      "drag down = ball goes up-screen" stays consistent. No aim correction needed after switching.
- [ ] **Ramp launch.** Putt hard up the ramp at the far end. The ball should launch into the air and arc down —
      and if it clears the map edge it **respawns** (it doesn't vanish for good).

### Tune the ball & shot live

Open the console (`` ` ``) and try these. After each, fire a putt to feel the change; reset with `-1`.

| Command | What it changes | Try |
|---|---|---|
| `pb.Ball.LinearDamping <v>` | How fast a rolling ball loses speed. **0.05** authored. | `0.2` = sticky/heavy; `0.01` = slidey/glassy |
| `pb.Ball.AngularDamping <v>` | How fast the ball stops **spinning**. **0.6** authored. | `2.0` = stops dead; `0.1` = spins forever |
| `pb.Ball.MaxImpulse <v>` | Full-power shot strength (cm/s·kg). **90** authored. | `200` = much farther; `50` = much shorter |
| `pb.Ball.MaxSpeed <v>` | Hard cap on horizontal speed (cm/s). **3000** authored. | `5000` = faster cap; `1500` = slower cap |
| `pb.Shot.MaxDragFraction <v>` | How far you drag (as a fraction of screen height) to reach **full** power. **0.28** authored. | `0.15` = full power with a small drag; `0.5` = must drag far |
| `pb.Shot.DeadZone <v>` | Drag length (pixels) below which release cancels. **24** authored. | `48` if short drags fire by accident (good on trackpads) |
| `pb.Shot.AtRestSpeed <v>` | Speed (cm/s) under which the ball counts as "stopped" so you can aim. **5** authored. | `20` = must be much stiller before aiming |
| `pb.Shot.RollTimeout <v>` | Seconds before a still-rolling ball is force-stopped (safety). **12** authored. | `5` if a stuck ball keeps you waiting |
| `pb.Shot.DrawDebugPreview <0/1>` | The debug aim line on/off. **1** (on). | `0` to hide it (it's placeholder until the real VFX preview lands) |

**Debug commands** (act on your ball; no `-1` needed):

| Command | Does | Tests |
|---|---|---|
| `pb.Ball.Dump` | Logs the ball's live values + all attribute flags to the Output Log. | — |
| `pb.Ball.SetScale <f>` | Resize the ball; mass scales with volume (×scale³). | `pb.Ball.SetScale 1.6` → noticeably **shorter** shots (heavier). `1.0` to reset. |
| `pb.Ball.Invert` | Toggle inverted aim (Reverse powerup preview). | drag direction flips |
| `pb.Ball.Lock` | Toggle shot-lock (Lock powerup preview). | aiming refuses |
| `pb.Ball.HideAim` | Toggle hidden aim line (Jumble powerup preview). | preview disappears |

### Persistent ball/shot tuning (when a console value feels right)

The console is for finding a value; to **keep** it, edit the authored default so it survives restart:

- **Ball:** open `Content/Ball/BP_BallPawn` → **Class Defaults** → category **PB|...**: `LinearDamping`,
  `AngularDamping`, `MaxSpeed`, `MaxImpulse`, `BaseMassKg` (0.045 ≈ a real golf ball), `BaseRadiusCm` (6).
- **Shot:** the same Blueprint hosts the shot component's `MaxDragScreenFraction` (0.28), `DeadZonePixels` (24),
  `AtRestSpeedThreshold` (5), `AtRestDuration` (0.5 s), `RollTimeout` (12), `PreviewLengthAtFullPower` (600),
  `PreviewMinLength` (50).
- **Power curve:** `PowerCurve` is a curve asset that shapes drag→power (currently **null = linear/1:1**). A
  gentle **S-curve** gives finer control on soft putts while still reaching full power — this is a feel artifact
  worth authoring during this pass.

> `BaseRadiusCm` is the one ball value that is **not** live-tunable (the collision sphere rebuilds at spawn) —
> change it on the Blueprint and re-play.

---

## Test 2 — Surfaces (Phase 2)

Putt the ball from the tee onto each band and watch how it rolls. The bands, in order across the lane, are
**Mud · Sticky · Fairway · Ice · Sand · Boost**. Expected behavior (the "roll multiplier" is the main knob —
**higher = shorter roll**; Fairway is the 1.0 baseline):

| Surface | Roll mult. | What the ball should do |
|---|---|---|
| **Fairway** | 1.0 | Baseline: rolls and slows believably. Everything else is judged against this. |
| **Ice** | 0.25 | Barely slows — long glide, slight skating, a little bounce. Should travel **~4×** the fairway. |
| **Sand** | 3.5 | Digs in and slows fast; short roll, almost no bounce. |
| **Sticky** | 6.0 | Grabs almost instantly — the shortest roll of all, no bounce. |
| **Mud** | 4.5 | Slows fast, between Sand and Sticky. (This one is the "added with **zero code**" proof surface.) |
| **Boost** | 1.0 | A placed **boost pad** *accelerates* the ball along an arrow while it's on the band, up to a speed cap. |

- [ ] Each surface is **clearly distinguishable** by feel alone — Ice, Sand, Sticky and Boost should be
      unmistakable. (This is the heart of the Phase 2 sign-off, UA‑10.)

### Tune surfaces live

| Command | What it changes | Try |
|---|---|---|
| `pb.Surface.DragCoefficient <v>` | A **global** multiplier on rolling drag across **all** surfaces. **1.5** authored. | `0.5` = everything rolls farther; `3.0` = everything grabs sooner; `-1` to reset |

This is the fast way to judge whether the *whole set* is too slidey or too draggy at once, before tuning
individual surfaces.

### Persistent per-surface tuning

To change **one** surface, open its definition asset and edit the values (the paired physical material mirrors
friction/restitution automatically — you don't edit it directly):

- `Content/Data/Surfaces/DA_Surface_Fairway` (and `_Ice`, `_Sand`, `_Sticky`, `_Boost`, `_Mud`)
  - **RollDragMultiplier** — the main roll knob (Fairway 1.0, Ice 0.25, Sand 3.5, Sticky 6.0, Mud 4.5).
  - **FrictionOverride / RestitutionOverride** — grip and bounciness (mirrored to the physics material on save).
- **Boost pad:** select the boost-volume actor in the level → Details → **PB|Surface|Boost**:
  `BoostAcceleration` (5000 cm/s²), `MaxBoostSpeed` (2500 cm/s), `ZoneExtent` (trigger size).
- **Surface drag globals** live on the ball's surface-sampler component: `RollDragCoefficient` (1.5),
  `GroundTraceSlack` (8), `MinDragSpeed` (2).

> **Regression check (no PIE needed):** the surface ordering has an automated test. **Tools → Test Automation**
> (Session Frontend) → run **`PuttBattle.Tests.Surfaces.RollDistanceOrdering`**, or from the console
> `Automation RunTests PuttBattle.Tests.Surfaces.RollDistanceOrdering`. It asserts Ice > Fairway > Sand > Sticky
> with Ice ≥ 4× Sticky. If you retune a surface and this goes red, your change broke the intended ordering.

---

## Test 3 — Hazards & checkpoints (Phase 2)

These share **one** respawn path: water, void, and falling off the world all send you to your **last checkpoint**
(or the tee). No stroke penalty. Test on `V_A`:

- [ ] **Water → respawn.** From the tee (no checkpoint passed yet), putt into the **water hazard** (over the sand
      band). The ball respawns **at the tee**, stopped.
- [ ] **Checkpoint progression.** Restart (**R**). Putt so the ball rolls **through checkpoint 0**, then into the
      water. It respawns **at checkpoint 0**, not the tee.
- [ ] **Never regress.** Continue past **checkpoint 1**, then into water → respawn **at checkpoint 1** (the
      highest you've reached). Going back over checkpoint 0 does **not** move your respawn backward.
- [ ] **Off the world.** Putt hard up the **ramp** so the ball flies off the map edge (below KillZ −500). Same
      respawn behavior as water — you reappear at your last checkpoint, you don't lose the ball.

Checkpoints and hazards are **placed actors**, so their tuning is per-actor in the level (not console CVars):
select one in the outliner and edit its Details — checkpoint `CheckpointIndex` / `RespawnZOffset` (10) /
activation radius (60), hazard box `extent` (100×100×50), or **World Settings → KillZ** (−500).

---

## Master tuning reference

Every live knob in one place. All CVars: `-1` = use the Blueprint-authored value, `≥ 0` overrides live.

**Ball physics** — `pb.Ball.LinearDamping` · `pb.Ball.AngularDamping` · `pb.Ball.MaxImpulse` · `pb.Ball.MaxSpeed`
**Shot feel** — `pb.Shot.MaxDragFraction` · `pb.Shot.DeadZone` · `pb.Shot.AtRestSpeed` · `pb.Shot.RollTimeout` · `pb.Shot.DrawDebugPreview` (1/0)
**Surfaces** — `pb.Surface.DragCoefficient`
**Debug commands** — `pb.Ball.Dump` · `pb.Ball.SetScale <f>` · `pb.Ball.Invert` · `pb.Ball.Lock` · `pb.Ball.HideAim`

**Persistent (asset/Blueprint) tuning:** `BP_BallPawn` (ball physics + shot constants + `PowerCurve`) ·
`DA_Surface_*` (per-surface roll/friction/restitution) · boost-volume actor · checkpoint/hazard actors ·
World Settings KillZ.

---

## Issues & tuning log (fill this in as you go)

Copy this into a scratch file (or edit it here) and jot everything down — we'll tackle the batch together.

**Feel issues**

| # | Area (ball / shot / surface:X / hazard / camera) | What I saw / what felt off | Severity (blocker / annoying / nitpick) |
|---|---|---|---|
| 1 | | | |
| 2 | | | |

**Tuning values I landed on** (so we can bake them into the assets)

| Knob (CVar or asset field) | Authored default | Value that felt right | Notes |
|---|---|---|---|
| `pb.Ball.LinearDamping` | 0.05 | | |
| `pb.Ball.MaxImpulse` | 90 | | |
| `pb.Shot.MaxDragFraction` | 0.28 | | |
| `pb.Surface.DragCoefficient` | 1.5 | | |
| _(add rows)_ | | | |

---

## Feel sign-off (the human gates)

These are **your** calls — they can't be automated, and they gate the phases:

- **UA‑9 (Phase 1 feel):** play the graybox and answer, on **both mouse and trackpad**:
  1. Does the ball slow like it has **weight** (not glassy, not syrup)?
  2. Can you make a **gentle, controlled short putt**?
  3. Does a **full-power** shot feel powerful?
- **UA‑10 (Phase 2 surfaces):** with the surfaces unlabeled, can you **name each one by feel alone**? Ice, Sand,
  Boost and Sticky should be obvious.

(Both are tracked in [USER-ACTIONS.md](../USER-ACTIONS.md). They stay "pending" until you sign off.)

---

## After this — Steam

Once the feel is dialed in and the issues batch is fixed, the remaining test is the **Steam two-instance**
session test (UA‑11 / UA‑12), which needs a packaged build and a second machine. That's documented in
[phase-03-testing.md](phase-03-testing.md) (Test B). Don't start it until single-player feel is signed off.

---

## Known caveats (expected, not bugs)

- **Graybox only** — no art, no sound, untextured surfaces. You're judging motion, not looks (art is Phase 10).
- **Debug aim line** — the red→green aim line is a placeholder (`pb.Shot.DrawDebugPreview`); the real spline/VFX
  preview is a later pass.
- **Camera zoom is FOV-based** — the 1/2/3 presets change angle + field-of-view, not a true dolly (D‑7); fine for graybox.
- **Stroke/score is placeholder** — the HUD counts strokes; full scoring is Phase 4.
- **The invisible-ball bug is multiplayer-only** — it's the lobby→match travel path (Phase 3), not single-player
  `V_A`. If the ball is missing here too, that's a real new finding — log it.
