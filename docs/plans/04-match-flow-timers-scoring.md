# Phase 4 — Match Flow, Timers & Scoring

**Context:** Phase 3 skeleton exists. This phase implements the full timer model (D8/D9/D10) and scoring (§2.4 of README) — the systems that define how the game *feels* competitively. All constants live in `UPBScoringConfig` + `FPBMatchSettings`; nothing hardcoded.

## T4.1 — Timer model
**Modify GameMode/GameState:**
- Playing starts with `Remaining = MapTime`. Replicate via server start-timestamp + duration (clients derive countdown; never replicate a ticking float).
- **First sink clamp:** on the first player's sink this hole, `Remaining = min(Remaining, FirstFinishClamp)`; multicast a "HURRY UP — 0:20" banner + SFX moment. Subsequent sinks change nothing.
- **ShotGrace:** at expiry, players whose ball is at rest are immediately DNF (finished=true, DNF flag). If ≥1 ball is still moving from a legal pre-expiry shot, enter `ShotGrace`: no new shots accepted; simulate until every moving ball rests or sinks (sinks still count, with TimeBonus 0); hard safety cap 15 s then force-stop. Then HoleResults.
- Early end: all players finished → straight to HoleResults (DF7).
**Done when:** scripted 3-client test demonstrates: clamp triggers correctly when first sink happens both before and after the 20 s mark; a ball launched at 0:01 sinks during ShotGrace and scores; at-rest players at expiry are DNF; banner shows once.

## T4.2 — Scoring + streaks
**Create:** `Scoring/PBScoringConfig` (PrimaryDataAsset: StrokePointValue=100, PositionBonus table 60/40/25/15/10/5, TimeBonusMax=30, AceBonusBase=75, AceStreakCap=5, SinkSpeedThreshold), `Scoring/PBScoreSubsystem` — **pure functions** taking (par, strokes, sink order, timeRemaining01, aceStreak) → breakdown struct. PlayerState: `ConsecutiveAces` (++ on strokes==1, reset to 0 otherwise; bonus = AceBonusBase × min(streak, cap)), per-hole breakdown array, match totals. DNF: zero hole score, streak resets. Tie-break comparator: total points → fewer total strokes → lower cumulative finish time.
**Tests (required):** Automation Spec fixtures: normal hole, DNF, ace, 6-ace run capping at ×5, tie-break ordering, ShotGrace sink (time bonus 0).
**Done when:** all fixtures pass; values changed in the config asset change results without recompiling.

## T4.3 — Results UI
**Create:** WBP_HoleResults (8 s: per-player breakdown rows — strokes, position, time, ace streak flair), WBP_Scoreboard (Tab: running totals + per-hole columns), WBP_Podium (match end: top-3 staging, full table, return-to-lobby). Streak presentation: flame counter on HUD while a streak is alive (it's a brag mechanic — make it visible to everyone).
**Done when:** a full 3-hole match shows correct math everywhere; a 2-streak ace displays its multiplier on all clients.
