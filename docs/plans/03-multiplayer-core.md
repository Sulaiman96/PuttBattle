# Phase 3 — Multiplayer Core

**Context:** Phases 0–2 complete offline. This phase makes it a game: authority, sessions, lobby. **First playtest happens at the end of this phase** — graybox, friends, real Steam.

> **Status (2026-06-17):** All four tasks' **C++ complete, compiling clean, unit-tested, and adversarially
> reviewed**; the **lobby content (L_Lobby map + WBP_Lobby UI + HUD/GameMode) is built and PIE-smoke-tested**
> (`docs/pr/phase-03-multiplayer-core.md`). Remaining is the human network validation: UA-11 (Steam client),
> UA-12 (two-instance Steam match), UA-13 (friends playtest). Deviations recorded in DECISIONS D-10…D-13.

## T3.1 — Authority & replication pass — **C++ done 2026-06-17** (PIE 3-client net-emulation run = human, UA-shaped)
**Goal:** Server-authoritative shots and smooth replicated balls.
**Modify:** ShotComponent routes release → `Server_RequestShot(FPBShotRequest)` `WithValidation`: phase == Playing/ShotGrace-not-allowed, ball at rest, `Power01 ≤ 1`, `!bShotLocked`. Server executes `ExecuteShot` (impulse, stroke++ on PlayerState).
- Ball physics replication: enable physics replication, set **Predictive Interpolation** for the autonomous proxy's ball; tune `NetUpdateFrequency` (ball 30 Hz while moving, 5 Hz at rest via dormancy-ish toggling), interpolation smoothing for simulated proxies.
- Verify ghost channel: two balls overlap freely; pickups/triggers still overlap.
- Local-feel: on release, owning client plays swing SFX/VFX immediately.
**Done when:** 3 PIE clients + listen server complete the test hole with `p.NetEmulation` 100 ms / 1 % loss; no rubber-banding on own ball; remote balls smooth; server rejects out-of-range/early shot requests (add a cheat-attempt automation or manual exploit test).

## T3.2 — Match framework skeleton — **C++ done 2026-06-17** (3-client phase-loop PIE run pending editor)
**Goal:** The phase machine that Phase 4 fills with timer rules.
**Create:** `Match/PBMatchGameMode` (server: phase transitions, hole sequencing stubs), `Match/PBMatchGameState` (replicated: `EPBMatchPhase`, server clock, current hole index, basic per-player finished flags), `Match/PBPlayerState` (strokes this hole, total score placeholder, checkpoint index, finished flag). Per-player input gating by phase (no shots outside Playing).
**Done when:** with 3 clients: HoleIntro → Countdown → Playing → (everyone sinks) → HoleResults → loops the same hole. Phases replicate and gate input correctly.

## T3.3 — Checkpoints & completion under replication — **C++ done 2026-06-17** (per-PlayerState tracking; server-stamped finish order)
**Modify:** CheckpointActor activation server-tracked per PlayerState; activation FX only on the owning client (RepNotify filtering). Cup sink: server-detected, marks PlayerState finished, records sink order + timestamp. Finished players get a placeholder fixed camera (real spectate is Phase 8 — keep the seam: route through `EnterSpectate()` stub).
**Done when:** two players verifiably respawn to *different* checkpoints; sink order is correct under latency (server timestamps, not client).

## T3.4 — Steam sessions & lobby — **C++ + editor content done 2026-06-17** (session subsystem + lobby GM/GS; L_Lobby map + WBP_Lobby UI + HUD built & PIE-smoke-tested; UA-12 two-instance Steam = human)
**Goal:** Create/browse/join over real Steam.
**Create:** `Net/PBSessionSubsystem` (GameInstance subsystem wrapping OSS: CreateSession w/ lobby metadata, FindSessions, JoinSession, friend invites/`join game` support, destroy/cleanup paths), `Maps/Core/L_Lobby` + lobby GameMode/State: seats (2–6), ready-up, host-only settings panel writing replicated `FPBMatchSettings { HoleCount(3–9), MapTimeSeconds, FirstFinishClampSeconds, TArray<FPBPowerupWeight> (wired in Phase 5) }`. Start → seamless travel into match flow.
- Robustness: handle join-in-lobby only (no join-in-progress in v1 — reject with reason), host-quit → all clients return to MainMenu with "Host left" message.
**Done when (agent):** with Steam running on the dev machine (UA-11), two local game instances create/find/join a session and complete a 3-hole match; host settings visibly in force; host-quit exits gracefully on the client. Real-network validation is human: UA-12.

## Phase exit
Human checkpoints UA-12 (two-machine Steam test) and UA-13 (first friends playtest) gate this phase. Agent prepares for UA-13: a packaged build, a one-page how-to-join, and a feedback sheet (fun of core roll/shoot, latency feel, anything confusing).
