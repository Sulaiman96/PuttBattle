# Phase 6 — Powerup Effects (17)

**Context:** Phase 5 framework complete. Every task here = 1 `UPBPowerupDefinition` asset + 1 `UPBPowerupEffect` subclass + VFX/SFX hooks.

**Implementation language per D21:** attribute-flag and one-call effects are implemented as **Blueprint** subclasses of `UPBPowerupEffect` using the helper API — **Jumble, Reverse, Glue, Lock, Balloon, Ice Age** — both to prove the BP extension path and so the human can read/tune them while learning. Physics, collision, validation, and world-actor effects stay **C++**: Mulligan, Anchor, Ghost Ball, Teleport, Shield, Shuffle, Swap, Ink, Gust, Tornado, Bumper (Tornado/Bumper actors get BP subclasses for visuals only). All durations, forces, and magnitudes are definition-asset or BP-default values — never C++ constants. All ∥ parallel-safe, but implement in the listed order — the first eight form a shippable fallback set if scope pressure hits. Every effect's Done-when includes: verified in 3-client PIE, Shield interaction correct for its category, banner/VFX readable by the victim.

## Self
**T6.1 Mulligan** (Instant). PlayerState records pre-shot transform + stroke at each `ExecuteShot`. Effect: teleport back, decrement stroke, clear velocity. Edge: nothing recorded this hole → refuse activation client-side (slot not consumed).
**T6.2 Anchor** (Armed). Sets `bAnchorArmed`. Pawn: on next blocking contact while armed → zero velocity, clear flag. Edge: armed while at rest → applies to next shot's first contact.
**T6.3 Ghost Ball** (Armed: next shot). Sets `bGhostShotArmed`. On that shot: ball's response to `PB_Wall` → Overlap (floors unchanged) until at-rest. At rest: validity check — sphere trace down must hit `PB_Floor` and ball must be inside course bounds volume; invalid → death FX + `RespawnAtCheckpoint`. Restore collision either way. Translucent ghost material while active.
**T6.4 Teleport** (PlaceLocation, radius-limited). Server validates: point on `PB_Floor`, within `PlacementRadius` of ball, not inside the cup trigger. Teleport, zero velocity. Does **not** count as a stroke.
**T6.5 Shield** (Passive/Armed). Sets `bShielded` + visible bubble on that ball (all clients — counterplay information). Consumed by the Phase-5 choke point. Does not stack (activating while shielded refuses).

## All-others
**T6.6 Jumble** (NextStroke). Victims get `bAimIndicatorHidden` — no preview line, no power bar, for their next executed shot.
**T6.7 Balloon** (Timed 12 s). Victims: `ScaleMultiplier 1.6`, `PowerMultiplier 0.6`; cup rejects per DF6; slight gravity scale down for floatiness. End restores; if ball ends overlapping a wall after deflate, depenetrate.
**T6.8 Reverse** (NextStroke). Victims: `bAimInverted` — ball travels along the drag, not opposite. Preview must show the *true* resulting direction? **No** — preview lies (shows normal aim); the surprise is the powerup. Document this choice in the asset description.
**T6.9 Shuffle** (Instant). Server snapshots all *other* active unfinished balls' transforms, derangement-shuffles them (no ball keeps its spot when n ≥ 2), teleports with a poof FX. Checkpoint states do NOT move with players.

## Single-target
**T6.10 Swap** (Instant). Caster and target swap positions + velocities zeroed. Blockable by Shield.
**T6.11 Lock** (Timed 6 s). `bShotLocked`; victim's HUD shows padlock + countdown; shot attempts shake the lock.
**T6.12 Ink** (Timed 8 s). Replicated state tag → victim's client renders full-screen ink splats (UMG overlay, holes to peek through). Purely presentational on victim's client; server only tracks duration.
**T6.13 Glue** (NextStroke). Victim `PowerMultiplier 0.4` for next executed shot; goo VFX on ball.

## Environment (affect caster too; usable while finished; ignore Shield)
**T6.14 Gust** (PickDirection, 6 s). World-space constant force (tuned ~ moves a resting ball slowly, bends rolling paths visibly) applied server-side to all active unfinished balls. Full-screen wind streak VFX aligned to direction on all clients.
**T6.15 Tornado** (PlaceLocationDirection, ~15 s). `PBTornadoActor` (server-spawned, replicated): patrols ± along the chosen axis from the placement point (3 passes, speed tuned), radial overlap flings balls (strong impulse + slight lift — one of the few legal vertical forces). Telegraph: dust ring marks the patrol path.
**T6.16 Ice Age** (Instant, 10 s). `SurfaceSubsystem.PushGlobalOverride(IceDef, 10)`. Screen frost vignette on all clients. Zero new physics code — if this needs more than the push call + VFX, Phase 2 failed.
**T6.17 Bumper** (PlaceLocation, 2 hits). `PBBumperActor`: replicated placed bumper; any ball contact → strong radial bounce + hit flash; 2 hits → break FX + despawn. Affects caster's ball equally. Server validates placement on floor, not inside cup/tee.

## T6.18 — Balance pass
After all 17, the **agent prepares the balance session (UA-14)**: lightweight local telemetry (per-powerup pickup/use/victim counts to CSV), a one-page survey sheet, suggested starting weights (Self 40 %, AllOthers 25 %, SingleTarget 20 %, Environment 15 % spread within categories), and a packaged build. After the human session, the agent applies tuning from the CSV + survey and records final defaults in the definition assets + `docs/BALANCE.md`.
**Done when:** tuning from UA-14 is applied and documented; no powerup's use-rate is an outlier in the telemetry without a written rationale.
