// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PBMatchTypes.generated.h"

/**
 * The match phase machine (CONVENTIONS §7 / plans/03 T3.2). Replicated on
 * APBMatchGameState; the server (APBMatchGameMode) owns every transition and
 * clients only read it to gate input + drive UI. Phase 4 fills in the timer
 * rules that move between these — Phase 3 only needs the states and the loop
 * HoleIntro → Countdown → Playing → HoleResults to exist and replicate.
 *
 * Modelled as an enum (not the Match.Phase.* tags) because the plan calls for a
 * replicated EPBMatchPhase and a small fixed-cardinality state is cheaper to
 * replicate and switch on than a tag. The tags remain the content-facing keys.
 */
UENUM(BlueprintType)
enum class EPBMatchPhase : uint8
{
	/** Pre-match lobby (seats, ready-up, host settings). */
	Lobby,
	/** ~4 s camera glance over the hole (D20). */
	HoleIntro,
	/** 3-2-1 countdown before control unlocks. */
	Countdown,
	/** Active play; the map timer ticks (filled in Phase 4). */
	Playing,
	/** Pre-expiry shots simulate to rest and may still score (D9). */
	ShotGrace,
	/** 8 s per-hole results before the next hole / podium. */
	HoleResults,
	/** Final standings / podium. */
	MatchResults
};

/**
 * True for phases where a player may legitimately take a shot (CONVENTIONS §2
 * input gating). Playing only: a ShotGrace shot must already be in flight from
 * before expiry (D9), so no *new* shot may start there. The server enforces this
 * in Server_RequestShot validation; clients use it to suppress the aim preview.
 */
inline bool PBPhaseAllowsShots(EPBMatchPhase Phase)
{
	return Phase == EPBMatchPhase::Playing;
}

/**
 * One powerup's relative spawn weight (DF5: relative weights, draw-time
 * normalisation). The host tunes these pre-match via sliders (Phase 5 wires the
 * UI + the draw); Phase 3 only needs the field in FPBMatchSettings so the
 * replicated settings struct is shape-stable across the lobby→match boundary.
 */
USTRUCT(BlueprintType)
struct FPBPowerupWeight
{
	GENERATED_BODY()

	/** Powerup identity (Powerup.* — CONVENTIONS §5). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Match", meta = (Categories = "Powerup"))
	FGameplayTag PowerupTag;

	/** Relative weight; only ratios matter (normalised at draw time, DF5). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Match", meta = (ClampMin = "0.0"))
	float Weight = 1.f;
};

/**
 * Host-configurable match rules (plans/03 T3.4). Written by the host in the
 * lobby, replicated to all clients, then carried into the match flow where
 * Phase 4's timer model reads MapTime/FirstFinishClamp and Phase 5 reads the
 * weights. Ranges follow DF1; HoleCount follows D11 (3–9, min 3).
 *
 * BlueprintReadWrite + EditAnywhere so the host-settings panel (UMG, editor
 * pass) binds straight to it without a code change, per the §1 tuning rule.
 */
USTRUCT(BlueprintType)
struct FPBMatchSettings
{
	GENERATED_BODY()

	/** Holes this match (D11: 3–9, minimum 3). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Match", meta = (ClampMin = "3", ClampMax = "9"))
	int32 HoleCount = 3;

	/** Per-hole base map timer in seconds (DF1: 60–300, default 90). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Match", meta = (ClampMin = "60.0", ClampMax = "300.0"))
	float MapTimeSeconds = 90.f;

	/** On first sink, remaining time clamps to this (DF1: 10–60, default 20; D8). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Match", meta = (ClampMin = "10.0", ClampMax = "60.0"))
	float FirstFinishClampSeconds = 20.f;

	/** Per-powerup relative spawn weights (wired in Phase 5; DF5). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Match")
	TArray<FPBPowerupWeight> PowerupWeights;
};
