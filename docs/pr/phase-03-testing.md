# Phase 3 — How to Test (multiplayer core + lobby)

A step-by-step guide for verifying everything Phase 3 added. Written assuming you've **never tested a
multiplayer build before** — exact menu paths and "what you should see" at each step (CONVENTIONS §12).

There are **two ways to test**, and you want both:

| | Needs Steam? | Needs 2 machines? | What it proves |
|---|---|---|---|
| **Test A — Local PIE** | No | No (one PC) | The match itself: phases, shots, replication smoothness, checkpoints, finish order, the hole loop. (T3.1/T3.2/T3.3) |
| **Test B — Steam two-instance** | Yes (UA-11) | Yes (UA-12) | The session layer: Host / Find & Join / friend invites over real Steam. |

Do **Test A first** — it's fast, needs no Steam, and catches the risky stuff. Test B (UA-12) validates the
Steam plumbing that PIE can't exercise.

---

## What Phase 3 added (so you know what you're looking for)

- **Server-authoritative shots** — you drag/release as before; the server runs the shot. Strokes now live on
  the player, replicated to everyone.
- **Match phase machine** — HoleIntro (≈4 s) → Countdown (3 s) → Playing → HoleResults (8 s) → next hole →
  after the configured number of holes → MatchResults.
- **Checkpoints & finishing** — each player respawns at *their own* last checkpoint; sinking marks you
  finished with a server-stamped order; finished players drop into a placeholder spectate state.
- **Lobby** — `L_Lobby` map with a graybox menu: **Host Game**, **Find & Join**, **Ready Up**, **Start Match**.

---

## Prerequisites

1. **Build the latest code.** Close the editor, then from the repo root run the build (the editor target).
   If the editor is open, the build is blocked — close it first. (Your agent normally does this; if you build
   yourself, it's the `PuttBattleEditor Win64 Development` target.) Re-open the editor afterward.
2. **(Test B only) Steam — UA-11:** the Steam client must be **running and logged in** on each machine.
   PuttBattle uses Steam's free dev app id **480 (Spacewar)**, so no Steam store registration is needed for
   testing. Just have Steam open and signed in before launching the game.

---

## Test A — Local match in PIE (no Steam, one PC) ✅ do this first

This runs the **listen server + multiple clients inside the editor**, so it exercises the real match flow and
replication without Steam. PIE auto-connects the clients, so you won't touch the Host/Find&Join buttons here —
you'll go straight to Ready + Start.

### A1. Set up multiplayer Play settings (one-time)
1. In the editor, find the **Play** button (green ▶ triangle) on the main toolbar.
2. Click the small **dropdown arrow next to it**.
3. Set **Number of Players** = **3** (type it in the field, or use the stepper).
4. Set **Net Mode** = **Play As Listen Server**.
   - If you don't see these in the dropdown: open **Edit → Editor Preferences → (Level Editor) Play →
     Multiplayer Options**, set **Number of Players = 3** and **Net Mode = Play As Listen Server** there.
5. (Recommended) Set the windows to spawn side by side: in those same Multiplayer Options, set the **new
   window size** to something small (e.g. 640×360) so you can watch all three at once.

### A2. Open the lobby map
1. **File → Open Level → `Content/Maps/Core/L_Lobby`** (or double-click `L_Lobby` in the Content Browser at
   `Content/Maps/Core/`).
2. You should see a flat grey floor. That's the graybox lobby — correct.

### A3. Play
1. Press **Play** (▶). Three game windows open. Each shows the **PUTT BATTLE — Lobby** title and four
   buttons (Host Game / Find & Join / Ready Up / Start Match) stacked in a corner.
   - *Why no Host/Find needed:* in PIE listen mode the three windows are already connected to each other, so
     skip those two buttons here (they're for the real Steam test).
2. In **each** window, click **Ready Up**.
3. In the **first** window (the listen-server host), click **Start Match**.
   - Nothing happens unless **all 3 are ready** and there are **≥2 players** — that's the lobby rule working.

### A4. Watch the match (this is the T3.1/T3.2/T3.3 verification)
After Start, all three windows travel to the graybox hole and the phase machine runs. Verify:

- [ ] **Phases advance** in all windows together: a brief intro, then a countdown, then play. (You can't shoot
      during the intro/countdown — that's the input gate.)
- [ ] **Shots are server-authoritative:** drag-release in one window; that ball moves in *all* windows.
- [ ] **Your own ball feels responsive** (no rubber-banding) and **other players' balls move smoothly** (no
      teleporting/stuttering).
- [ ] **Stroke counter** on the HUD increments when you shoot, and is correct in each window for that player.
- [ ] **Checkpoints:** roll a ball through a checkpoint, then into water/off the edge — it respawns at the
      checkpoint it passed, **not** the tee. Two different players can sit at two different checkpoints.
- [ ] **Sinking:** put a ball in the cup → that player is marked finished. When **everyone** has sunk, the
      hole ends (HoleResults), then the **next hole** starts (it's the same graybox hole replayed — variant
      maps are Phase 7).
- [ ] **Match end:** after the configured number of holes (default 3), it reaches MatchResults and stops.

### A5. (Optional) Test under bad-network conditions
The Phase-3 "done when" calls for 100 ms latency / 1 % loss. With PIE running, open the console (press the
**`~`** key in a game window) and type:
```
p.NetEmulation.PktLag 100
p.NetEmulation.PktLoss 1
```
Repeat A4's "smoothness" checks — remote balls should still interpolate smoothly, no authority warnings in
the Output Log.

> **If a window gets stuck and nobody can sink:** the hole still ends when the map timer runs out (a Phase-3
> safety fallback). Press **Esc** / Stop to end PIE.

---

## Test B — Steam two-instance (UA-12) — needs Steam + a second PC/account

This is the only way to test **Host / Find & Join / friend invites** over real Steam. You need **two machines**
(or a friend) — Steam won't run two instances of the same app id on one account.

### B1. Package a build (so it runs outside the editor)
1. **Platforms (toolbar icon) → Windows → Package Project.** (First time: it may ask you to pick a target —
   choose **Development**.)
2. Pick an output folder when prompted. Packaging takes several minutes; a progress bar shows bottom-right.
3. When done, the build is in your chosen folder under `Windows/PuttBattle.exe`. Copy that whole `Windows`
   folder to the second machine (or share it with your friend).

### B2. Run the test (both machines)
1. **Both:** make sure **Steam is running and logged in** (UA-11).
2. **Both:** launch `PuttBattle.exe`. Each lands on the **lobby menu** (Host / Find & Join / Ready Up / Start).
3. **Machine 1 (host):** click **Host Game**. It creates a Steam room and moves you into the lobby (you stay
   on the lobby screen as the listen server).
4. **Machine 2 (joiner):** click **Find & Join**. It searches Steam, finds machine 1's room, and connects you
   into machine 1's lobby (about a 1-second search delay is normal).
   - *Friend-invite alternative:* on machine 1, press **Shift+Tab** (Steam overlay) → **Friends** → right-click
     your friend → **Invite to Game**. They accept and join the same way.
5. **Both:** click **Ready Up**.
6. **Machine 1 (host):** click **Start Match**. Both travel into the hole and play a full 3-hole match.

### B3. Verify (Steam-specific)
- [ ] Machine 2 successfully **finds and joins** machine 1's room.
- [ ] Both complete a **3-hole match** together.
- [ ] **Host settings are in force** (if you change hole count / map time on the host before starting, the
      match honors them).
- [ ] **Host quits mid-match** → the joiner is returned to the menu (a "host left" bounce), not left hanging.

---

## Packaging vs. quick checks — summary

- **Just want to see the lobby UI?** Open `L_Lobby`, press Play (single player). The menu appears. (Already
  smoke-tested by the agent — the widget shows on screen.)
- **Want to test the match locally?** Test A (PIE, 3 players, listen server). No Steam.
- **Want to test Steam sessions?** Test B (packaged build, two machines). Needs UA-11 + UA-12.

## Known caveats (expected, not bugs)
- **Graybox only** — the lobby UI is unstyled (corner buttons), no art/sound yet (that's Phase 10).
- **Find & Join joins the *first* room found** — fine for a 1-host test; a full server-browser list is a
  later polish pass.
- **No join-in-progress** — you can only join during the lobby, not after the match starts (by design, v1).
- **One machine, one Steam instance** — the Steam path (Test B) genuinely needs two machines/accounts; use
  Test A for single-PC verification.

## Where the human gates live
- **UA-11** (Steam running), **UA-12** (this two-instance test), **UA-13** (friends playtest) are tracked in
  `docs/USER-ACTIONS.md`. This document is the detailed how-to for UA-12; UA-13 reuses Test B with real friends
  plus a feedback sheet (ask the agent to generate one when you're ready).
