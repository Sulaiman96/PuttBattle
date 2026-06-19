# PR: Phase 3 — Multiplayer Core (C++)

**Branch:** `phase/03-multiplayer-core` · **Date:** 2026-06-17 · **Status:** C++ complete & compiling
(editor target, **zero new warnings**); both automation tests pass. **Lobby map + functional UI built and
PIE-smoke-tested (2026-06-17).** Only the human network tests (UA-11/12/13) remain — see **Remaining (human)**.

Implements `docs/plans/03-multiplayer-core.md`: server-authoritative shots, smooth replicated balls, the
match phase machine, checkpoints/completion under replication, and the Steam session + lobby layer. Built
without the editor bridge (it was down all session), so all asset/map/UMG work is deferred and listed below;
the C++ is the full, testable backbone.

> **Feel-testing safety:** the multiplayer mode is **never** forced onto the offline test map `V_A`. The
> match/lobby GameModes are applied via the `?game=` travel URL (D-11), and shot-gating stays permissive when
> there is no match GameState — so the human's Phase 1–2 feel-testing on `V_A` is unaffected by this branch.

## What shipped

### T3.1 — Authority & replication pass
- **Server-authoritative shot.** `UPBShotComponent` release now routes through `Server_RequestShot`
  (`Server, Reliable, WithValidation`); the listen-server host runs the same server path directly. `_Validate`
  is a cheat guard (finite dir/power, power∈[0,1]+slack — a fail disconnects); state gates (phase, ball-at-rest,
  Lock, finished, ownership) are re-checked in `ServerShotStateAllows()` and **silently** rejected so honest
  latency never kicks a player. `ExecuteShot` (impulse + `PlayerState->AddStroke()`) is the single server funnel.
- **Double-shot guard.** On release the component enters a Rolling lock held by `bWaitingForShotMotion` until
  the ball is actually observed to move (or a `ShotConfirmTimeout` fallback), so a queued shot can't be
  re-fired before the first replicates back. Roll-timeout force-stop is now authority-only.
- **Physics replication.** Ball uses `EPhysicsReplicationMode::PredictiveInterpolation` (smooth simulated
  proxies) + `bReplicatePhysicsToAutonomousProxy=true` (corrects the owning client). Net cadence throttles
  30 Hz moving / 5 Hz at rest via `SetNetUpdateFrequency` (a net-rate throttle, **not** dormancy — D-12), using
  the 5.7 deprecation-safe setters.
- **Local feel.** `OnLocalShotFired(Power01)` BP hook fires the instant the player releases (swing SFX/VFX),
  before the server round-trip.

### T3.2 — Match framework skeleton
- `Match/PBMatchGameMode` (`: APBGameMode`) — server phase machine: `Warmup → HoleIntro → Countdown → Playing
  → (all sink | map-time fallback) → HoleResults → next hole / MatchResults`. Single reused `PhaseTimer`;
  per-hole reset of all players; durations are `EditAnywhere` (CONVENTIONS §7 defaults).
- `Match/PBMatchGameState` (`: AGameStateBase`, D-10) — replicates `EPBMatchPhase`, a server-time phase
  deadline (drift-free clock via `GetServerWorldTimeSeconds()`), current hole index, and the active
  `FPBMatchSettings`. Helpers: `GetPhaseTimeRemaining`, `ArePlayerShotsAllowed`, `AreAllPlayersFinished`.
- `Match/PBPlayerState` (`: APlayerState`) — strokes (server-authoritative), total-score placeholder,
  checkpoint index, finish flag/order/server-time, lobby ready flag. Carries across seamless travel via
  `CopyProperties` (ready flag deliberately does **not** carry).
- **Input gating** is authoritative in the shot RPC (reads the replicated phase), not client `SetIgnoreInput`.

### T3.3 — Checkpoints & completion under replication
- Checkpoint activation is server-tracked **per PlayerState** (`CheckpointIndex`), replacing the base mode's
  per-ball map; respawn resolves the checkpoint actor by index (D-13). Activation FX stays owning-client-only.
- Cup sink (already authority-gated) calls `GameMode->NotifyBallSunk`: the match mode stamps **server**
  finish order + `GetServerWorldTimeSeconds()` (authoritative ordering under latency), routes the player into
  the spectate seam, and ends the hole early once everyone is in (DF7).
- **Spectate seam:** `APBPlayerController::EnterSpectate()/ExitSpectate()` (server) → `Client_*` →
  `OnEnterSpectate/OnExitSpectate` BP hooks (placeholder fixed cam; real spectate is Phase 8). Players are
  pulled out of spectate at each hole start.

### T3.4 — Steam sessions & lobby
- `Net/PBSessionSubsystem` (`UGameInstanceSubsystem`, survives travel) — OSS wrapper: `CreateRoom` /
  `FindRooms` / `JoinRoomByIndex` / `DestroyRoom` + Steam friend-invite/"Join Game" (`FOnSessionUserInviteAccepted`)
  + host-left handling. Uses the 5.7-correct API (verified against engine source): presence==lobby flags,
  `SEARCH_LOBBIES` (not the removed `SEARCH_PRESENCE`), `Add*Delegate_Handle`/`Clear*` lifecycle, host
  `ServerTravel(?listen)` vs client `GetResolvedConnectString`+`ClientTravel`. Carries the host's
  `FPBMatchSettings` into the match (GameInstance scope outlives the GameState).
- `Match/PBLobbyGameMode` + `Match/PBLobbyGameState` — seats (2–6), host tracking (first login), host-only
  settings (clamped to D11/DF1 ranges), ready-up tally, and `RequestStartMatch` → seamless `ServerTravel` into
  the match map (`?game=` forces the match mode). Lobby RPCs on the PlayerController (`Server_SetReady`,
  `Server_SetMatchSettings`, `Server_RequestStartMatch`) are `WithValidation`.
- `FPBMatchSettings` / `FPBPowerupWeight` / `EPBMatchPhase` in `Match/PBMatchTypes.h` (weights wired in Phase 5).
- Deleted the obsolete `Net/PBNetPlaceholder.h` (a real Net class now lives in the folder).

## Verification evidence
- **Build:** `PuttBattleEditor Win64 Development` (editor closed) → **Result: Succeeded**, `UnrealEditor-PuttBattle.dll`
  linked, UHT `-WarningsAsErrors` passed, **zero warnings in `PuttBattle`** (only the pre-existing StructUtils
  plugin-deprecation notices from the vendored bridge remain).
- **Automation:** `PuttBattle.Tests.Shot.RequestValidation` → **Result={Success}** (new — the T3.1
  "cheat-attempt" test: honest shots pass; over-power/negative/NaN/Inf payloads rejected) and
  `PuttBattle.Tests.Surfaces.RollDistanceOrdering` → **Result={Success}** (`Ice=5303.4 Fairway=1313.4
  Sand=363.4 Sticky=205.0`). `EXIT CODE: 0`.
- **Adversarial review:** a 4-dimension review (replication/authority, game logic, conventions, sessions) with
  per-finding verification surfaced **10 confirmed issues**; **9 fixed** this branch (incl. one *critical* — a
  host-side `OnNetworkFailure` that would tear the whole match down when any client timed out under the
  `p.NetEmulation` test; now gated to client-only), **1 documented** (below). Re-built + re-tested clean after.
- **PIE 3-client + net-emulation** (T3.1/T3.2/T3.3 done-when) and **two-instance Steam** (T3.4 done-when)
  require the editor + lobby content — they are the **Requires editor** / human steps below.

## Known issues / deliberate simplifications
- **Mid-hole late registrant (low):** a player whose PlayerState registers after a hole's reset would be
  counted by `AreAllPlayersFinished` (no early-end). It **cannot hang** the match — the `OnPlayingExpired`
  map-time fallback still advances the hole — and v1 disables join-in-progress (`bAllowJoinInProgress=false`),
  so it's largely unreachable. Full DNF handling for late joiners lands with Phase 4's timer/DNF model.
- **Playing-phase expiry is a stub:** Phase 3 ends Playing on all-sink, with a map-time fallback to avoid
  hangs. The real expiry → **ShotGrace** (in-flight shots simulate to rest, D9) is Phase 4.
- **Hole sequencing is a stub:** the loop replays the same graybox hole `HoleCount` times; real variant
  streaming is Phase 7.
- **Net dormancy:** intentionally a rate-throttle, not true dormancy (D-12).

## Editor content built this session (2026-06-17, via ue-mcp)
- **`/Game/Maps/Core/L_Lobby`** — graybox lobby map (floor, sun + skylight, PlayerStart). World Settings
  GameMode = `BP_PBLobbyGameMode`. `GameDefaultMap` repointed here (was V_A — the D-8 stopgap), so a
  standalone/packaged launch lands on the lobby menu. V_A's own World Settings are **unchanged** (still the
  offline referee for feel-testing).
- **`/Game/UI/WBP_Lobby`** — functional graybox lobby widget: **Host Game** → `UPBSessionSubsystem::CreateRoom(6,"PB Room")`;
  **Find & Join** → `FindRooms(50)` → 1 s delay → `JoinRoomByIndex(0)` (joins the first room — enough for the
  two-instance test; a full server-browser list is the polish follow-up); **Ready Up** → `Server_SetReady(true)`;
  **Start Match** → `Server_RequestStartMatch`. Compiles clean (0 errors/0 warnings).
- **`/Game/Match/BP_PBLobbyHUD`** — `BeginPlay → CreateWidget(WBP_Lobby) → AddToViewport`. The lobby PC
  (`APBPlayerController`) already enables cursor + GameAndUI input (Phase 1), so the buttons are clickable.
- **`/Game/Match/BP_PBLobbyGameMode`** — `APBLobbyGameMode` subclass carrying `HUDClass = BP_PBLobbyHUD`.
- **C++ tweak (Live-Coding-compiled):** the lobby `ServerTravel` no longer forces `?game=` — the lobby mode
  now comes from L_Lobby's World Settings (so host + joining clients share the same lobby mode + HUD). The
  *match* hop still forces `?game=/Script/PuttBattle.PBMatchGameMode` (keeps V_A on its offline referee). See D-11.
- **PIE smoke test:** standalone PIE on L_Lobby → `WBP_Lobby` confirmed `inViewport`/visible; no stray pawn
  (lobby is UI-only). The host/join/ready/start *Steam* paths still need UA-12 (Steam + two instances) to validate.

## Still deferred (optional / not blocking)
- **Server-browser list** (one row per found room) — current UI joins the first room found, which covers the
  two-instance test; a scrollable list is a polish pass.
- **Transition map** for seamless travel — `[/Script/EngineSettings.GameMapsSettings] TransitionMap=` is unset,
  so the engine uses its default empty level (works; a dedicated `L_Transition` just gives a nicer hop).
- **BP graybox seam hooks** — `OnLocalShotFired` (swing FX), `OnEnterSpectate`/`OnExitSpectate` (placeholder
  cam), `OnFinishedChanged`. The C++ seams fire; these BP implementations are cosmetic and can wait for art (Phase 10).
- **Mid-hole late-registrant** early-end edge (review finding #8) — mitigated by `bAllowJoinInProgress=false`
  and the map-time fallback; full DNF handling lands with Phase 4.

## Remaining (human)
> **Test guide: `docs/pr/phase-03-testing.md`** — exact steps for the local PIE match test (no Steam), the
> Steam two-instance flow, packaging, and a per-feature verification checklist.
- **UA-11** Steam client running on the dev machine (dev AppId 480).
- **UA-12** Two-machine / two-instance Steam test: create/find/join + a 3-hole match; host settings in force;
  host-quit exits gracefully on the client.
- **UA-13** First friends playtest (graybox). Agent prepares a packaged build + how-to-join + feedback sheet
  once the lobby UI exists.
