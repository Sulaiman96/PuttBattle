# Phase 9 — Progression & Cosmetics

**Context:** Match loop complete. This phase adds the reasons to keep playing. No backend: SaveGame + Steam Cloud, Steam Achievements. Cheating only affects the cheater's own cosmetics — accepted (D17/D19).

## T9.1 — Save system + Chips
**Create:** `Progression/PBSaveGame` (Chips balance, unlocked cosmetic tags, equipped loadout, achievement progress counters, host's last lobby settings incl. powerup weights), `Progression/PBProgressionSubsystem` (GameInstance): load-on-boot, autosave on match end + lobby settings change; Steam Cloud enabled for the save slot. Chips award = `floor(MatchScore × ChipRate)` + flat participation amount; values in ScoringConfig.
**Done when (agent):** Chips persist across restarts; Steam Cloud is enabled and the save slot syncs (verify via Steam's local cloud logs). Two-machine roam confirmation is human (UA-16).

## T9.2 — Cosmetics ∥
**Create:** `Progression/PBBallCosmeticDefinition` (slot: Material | Trail | Hat; refs; unlock: ChipPrice **or** Achievement tag), customiser screen in MainMenu/Lobby (live ball preview, equip/purchase), equipped loadout replicated via PlayerState → cosmetic mesh/trail/hat applied on all clients (hat: socketed, no collision, scales with Balloon for comedy). v1 content: 10 materials, 4 trails, 4 hats.
**Done when:** a player's cosmetics render correctly on every client including during Balloon; purchase/equip persists.

## T9.3 — Achievements ∥
**Create:** `Progression/PBAchievementDefinition` (tag, Steam API name, display, trigger: stat threshold), stat tracking hooks (matches won, total aces, 5-streak ace, hole-in-one with Ghost Ball, win without using powerups, all 17 powerups used, etc. — author ~15), Steam Stats/Achievements sync via OSS Achievements interface. In-game toast on unlock.
**Done when:** achievements unlock in-game and appear in the Steam overlay (dev AppId limits noted — full verification at T11.1 with the real AppId).
