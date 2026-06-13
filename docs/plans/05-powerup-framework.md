# Phase 5 — Powerup Framework

**Context:** Phases 0–4 complete. This is the project's biggest framework phase. Build it exactly to the extensibility contract: after this phase, **each of the 17 effects in Phase 6 is one definition asset + one effect class**, and nothing here changes again.

## T5.1 — Definitions & types
**Create:** `Powerups/PBPowerupDefinition` (PrimaryDataAsset): `PowerupTag`, display name/icon/description, `DefaultWeight` (int 0–100), `EPBTargetingRule { Self, SingleTarget, AllOthers, Environment }`, `EPBActivationMode { Instant, PickDirection, PlaceLocation, PlaceLocationDirection }`, `EPBDurationType { Instant, Timed, NextStroke, Armed }` + duration seconds, `TSubclassOf<UPBPowerupEffect>`, `bUsableWhileFinished` (true only for Environment by convention), placement params (radius for Teleport-likes, validity rule: RequiresFloorPoint).
**Done when:** asset type exists with full editor UX (categories, tooltips); a registry (asset manager primary asset type `PowerupDefinition`) enumerates all definitions.

## T5.2 — Effect runtime
**Create:** `Powerups/PBPowerupEffect` (server-instanced UObject, **Blueprintable**): `Begin(Context)` / `End()` as `BlueprintNativeEvent`s, plus a sanctioned **BlueprintCallable helper API** so simple effects are authorable entirely in BP without touching replication: `ModifyTargetAttributes(Target, FPBBallAttributes Delta)`, `ApplyImpulseToTarget`, `TeleportTarget`, `PushGlobalSurfaceOverride`, `SpawnReplicatedEffectActor`, `GetActiveBalls`. BP effects run server-side only by construction — the framework owns all replication and the Shield choke point regardless of implementation language (D21); `FPBEffectContext { Caster PS, Targets array, Location, Direction, Def }`. `Powerups/PBEffectManagerComponent` on GameState (server): instantiates effects, tracks Timed/NextStroke/Armed lifetimes (NextStroke effects end after the affected player's next executed shot; Armed effects end when consumed), replicates active `Effect.State.*` tags per player for UI/VFX.
- **Targeting resolution:** Self → caster; SingleTarget → payload target (validate active+unfinished); AllOthers → all active unfinished players except caster; Environment → all active unfinished players **including caster**.
- **Shield rule (single choke point):** before applying a SingleTarget/AllOthers effect to a shielded player, consume shield, cancel that application, multicast "BLOCKED". Environment bypasses entirely.
**Done when:** a debug `LogEffect` powerup flows end-to-end for every targeting rule in 3-client PIE; shield blocks the right categories and only those.

## T5.3 — Inventory & pickups
**Create:** `Powerups/PBPowerupInventoryComponent` on **PlayerState** (survives pawn churn + persists across holes per D6): replicated 3-slot FIFO of PowerupTags; `Server_ActivateSlot(Index, FPBTargetPayload { TargetPS?, Location?, Direction? })` with validation (owns it, phase ok, finished-players-only-Environment rule, payload sane for the activation mode). `Powerups/PBPickupActor`: bobbing box, overlap by ball → weighted draw → grant (full inventory = no grant, box stays), 10 s respawn, multicast collect FX.
- **Weighted draw:** from `FPBMatchSettings.PowerupWeights` (raw ints), normalised at draw time. Pure function + Automation tests (zero-weight excluded, all-zero falls back to defaults, distribution sanity over 10k draws).
**Done when:** rolling through grants correct slot behaviour in multiplayer; weights of {0} powerups never drop; tests pass.

## T5.4 — Targeting & placement UI
**Goal:** One reusable interaction flow for all four activation modes — the most complex UI in the game.
**Create:** `UI/PBTargetingController` (state on PlayerController): activating a slot enters its mode →
- `Instant`: fire immediately.
- `PickDirection`: world-space arrow from your ball follows cursor; click confirms; RMB/Esc cancels.
- `PlaceLocation`: ghost preview mesh under cursor; validity = floor raycast (+ within radius of ball when Def says so, e.g. Teleport); green/red tint; click confirms.
- `PlaceLocationDirection`: place, then drag an axis arrow; confirm on release.
- SingleTarget: hovering enemy balls highlights; click selects; also Tab-cycles. Show name plate.
Time pauses for no one — you can be hit while aiming a powerup. Cancelling returns the slot unconsumed; the slot is consumed only when the server accepts activation.
**Done when:** each mode works with a debug effect in multiplayer at 100 ms emulated latency (ghost preview is client-side; server re-validates placement on activation).

## T5.5 — Lobby weight sliders + powerup HUD
**Create:** lobby panel listing every registered powerup: enable toggle + weight slider (0–100) + **live computed % of total** (DF5: relative weights, normalised at draw — sliders never fight each other); Reset to defaults; host-only editing, replicated live to all lobby clients; persisted in host's SaveGame. In-match HUD: 3 slots with icons/keys, active incoming-effect banners from `Effect.State.*` tags ("JUMBLED!", shield-block flash), spectator-mode greying of non-Environment slots (full spectate in Phase 8).
**Done when:** host slider changes are visible to all lobby members and provably alter in-match drop rates; non-host cannot edit.
