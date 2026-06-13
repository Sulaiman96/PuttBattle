# Phase 8 — Spectator System

**Context:** Until now finished players sit on a placeholder camera (T3.3 stub `EnterSpectate()`). This phase implements D13 fully. Depends on Phase 5/6 for the powerup rule.

## T8.1 — Spectate cycling
**Modify/Create:** On finish (or DNF at results), PlayerController enters spectate state: view through the *hole's camera rigs* (reuse PBCameraSubsystem) with a followed-target context — ←/→ cycles target among active unfinished players, C still cycles rigs. Target's name + stroke count + held-powerup count on the spectate HUD. Auto-retarget when your target finishes.
**Done when:** 3 clients: two finished players independently cycle targets smoothly; mid-cycle target finishing retargets gracefully.

## T8.2 — Spectator visibility (being watched)
**Create:** replicated spectator map (GameState: who watches whom — updated via `Server_SetSpectateTarget`). Active players' HUDs render a spectator strip: small eye icon + name per watcher. Joining/leaving watchers animate in/out.
**Done when:** the watched player sees an accurate live list; switching targets moves your eye icon within ~1 s at emulated latency.

## T8.3 — Powerups while finished
**Modify:** spectate HUD shows the player's remaining inventory; Environment powerups activatable (targeting UI works from spectate cameras — placement raycasts use the spectate view), all other categories greyed with tooltip. Server enforces the rule regardless of UI (already in T5.3 validation — verify the path).
**Done when:** a finished player drops a Tornado on the leader from spectate; a hacked-style direct RPC attempt to use a SingleTarget powerup while finished is rejected server-side.
