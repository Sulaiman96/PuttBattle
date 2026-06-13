# Phase 10 — Art, Audio & Juice

**Context:** The game is fully playable in graybox. This phase is iterative passes, not new systems. Low-poly diorama per D18. Tasks ∥ where marked.

## T10.1 — Modular environment kit
**Input (human, UA-17):** the chosen low-poly kit dropped in `RawAssets/Kit/` — agent imports via `manage_asset`, organises into `Content/`, and does all dressing. Kit requirements to give the human up front: floor tiles, wall segments, ramps, curves, tubes, bumper rails, decorative props per theme; **surface colour language is a readability requirement** — fairway green, ice pale blue, sand yellow, sticky purple, boost chevrons, hazard water blue: a new player must read surfaces at a glance from the isometric camera. Re-dress the 18 graybox variants with the kit (geometry/collision unchanged — validator re-run after every map touched).
**Done when (agent):** all variants dressed; validator green; surface colour language consistent across maps. Fresh-eyes readability is human (UA-19).

## T10.2 — VFX/SFX pass ∥
**Input (human, UA-18):** licensed SFX files in `RawAssets/Audio/` — agent imports and wires all of it. Needed events: shot (charge tick, thwack), roll-by-surface loops, sink (confetti + crowd pop, bigger on aces/streak), checkpoint chime (owner-only), respawn splash/poof, each powerup's cast/victim/world FX (Tornado, Gust streaks, Ice frost, Ink splats, Shield pop, Bumper boing), HURRY-UP klaxon + ShotGrace heartbeat, countdown beeps, podium fanfare.
**Done when:** an observer can narrate a match with sound off-screen — every event is audible-distinct.

## T10.3 — UI skin + menus ∥
Visual pass over HUD/lobby/results/customiser to a coherent chunky-playful style; MainMenu scene (ball on a diorama green); settings (audio/video/keybinds); controller-aware later — mouse-first v1.

## T10.4 — Music ∥
Menu/lobby theme, 2–3 in-match loops (intensity layer keyed to HURRY-UP), podium sting. **Tracks supplied by human (UA-18)** — licensing is a budget/legal decision; agent implements the intensity layering and wiring. Placeholder-legal until T11.

## T10.5 — Juice pass
Screen shake (Gust/Bumper/Tornado hits), ball squash-stretch on impacts, cup-lip wobble on near-misses, spectator eye icons pop, ace-streak flames grow per streak, kill-feed-style event ticker ("Sulaiman → Ink → Alex").
**Done when:** side-by-side capture vs pre-juice build is night-and-day; no effect obscures aiming.
