# Phase 11 — Ship

## T11.1 — Steamworks (real)
Real AppId, depots + branches (default/beta), achievements/stats schema uploaded, store assets (capsules, screenshots, trailer), rich presence ("Playing hole 4/9"), invite/join flows re-verified on the real AppId, Steam Input not required (mouse-first) but verify no conflicts.

## T11.2 — Playtest rounds
Structured rounds: (a) friends graybox-art mix, (b) closed beta branch 10–20 players. Instrument lightweight local telemetry (per-hole strokes, DNF rate, powerup pick/use rates) written to CSV for tuning — no backend. Tune: ScoringConfig, default weights, MapTime defaults, physics feel. Exit criteria: median session ≥ 2 consecutive matches; no hole with DNF rate > 30 %; no powerup with use-rate < 3 %.

## T11.3 — Performance & stability
Targets: 120 fps on a GTX 1660-class machine at 1080p (low-poly should crush this — verify Niagara and 6-ball worst cases), packaged size < 2 GB, no GC hitches at hole transitions (level-instance streaming audit), 4-hour listen-server soak without leak growth.

## T11.4 — Launch checklist
Pricing (cheap one-time per D17 — decide regionally via Steam's matrix), age ratings questionnaire, EULA/privacy (no data collection to declare), build review pass, day-one patch branch ready, post-launch backlog seeded (host migration, EOS crossplay, more powerups/slots — explicitly out of v1 scope).

## Human/agent split (this phase is human-heaviest — see USER-ACTIONS.md)
Yours: Steamworks account + fees + tax/bank (UA-20 — **start during Phase 8**, verification takes weeks), store asset approvals (UA-21), pricing + ratings confirmation (UA-22), beta recruiting/community (UA-23), running credentialed uploads (UA-24).
Agent's: everything draftable or scriptable — store copy, screenshot/trailer footage candidates, ratings questionnaire drafts, SteamPipe scripts, depot config, telemetry analysis, tuning patches, perf passes, launch checklist upkeep.
