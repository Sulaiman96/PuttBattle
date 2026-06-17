// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "PBMatchTypes.h"
#include "PBMatchGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPBMatchPhaseChanged, EPBMatchPhase, NewPhase);

/**
 * The replicated facts of a match everyone sees (LEARNING.md quartet): the
 * current phase, when the phase ends (in server time, for a drift-free clock),
 * which hole is in play, and the active host settings. The server
 * (APBMatchGameMode) owns every write; clients read these to gate input and
 * drive UI. OnRep_* only updates presentation (CONVENTIONS §2).
 *
 * Subclasses AGameStateBase — NOT AGameState — deliberately: AGameState carries
 * Epic's auto-driven MatchState FSM (WaitingToStart/InProgress/…) which would
 * fight our own EPBMatchPhase machine. We own the phase transitions (D-aligned),
 * so the lean base is correct.
 */
UCLASS()
class PUTTBATTLE_API APBMatchGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "PB|Match")
	EPBMatchPhase GetMatchPhase() const { return MatchPhase; }

	UFUNCTION(BlueprintPure, Category = "PB|Match")
	int32 GetCurrentHoleIndex() const { return CurrentHoleIndex; }

	UFUNCTION(BlueprintPure, Category = "PB|Match")
	const FPBMatchSettings& GetMatchSettings() const { return MatchSettings; }

	/** Seconds until the current phase ends (server time; 0 once elapsed). Drift-free
	 *  on clients because PhaseEndServerTime is compared against the replicated clock. */
	UFUNCTION(BlueprintPure, Category = "PB|Match")
	float GetPhaseTimeRemaining() const;

	/** True only during Playing (and ShotGrace for in-flight shots) — UI/aim gating. */
	UFUNCTION(BlueprintPure, Category = "PB|Match")
	bool ArePlayerShotsAllowed() const { return PBPhaseAllowsShots(MatchPhase); }

	/** True when every connected player has sunk this hole (drives HoleResults). */
	UFUNCTION(BlueprintPure, Category = "PB|Match")
	bool AreAllPlayersFinished() const;

	/** Fired when the phase replicates (clients) or is set (server) — UI binds here. */
	UPROPERTY(BlueprintAssignable, Category = "PB|Match")
	FPBMatchPhaseChanged OnMatchPhaseChanged;

	// --- Server writes (authority only; called by APBMatchGameMode) --------

	/** Set the phase and its server-time deadline (Duration <= 0 = no timed end). */
	void SetMatchPhase(EPBMatchPhase NewPhase, float Duration);

	void SetCurrentHoleIndex(int32 NewIndex);

	void SetMatchSettings(const FPBMatchSettings& NewSettings);

protected:
	UFUNCTION()
	void OnRep_MatchPhase();

private:
	UPROPERTY(ReplicatedUsing = OnRep_MatchPhase, Transient)
	EPBMatchPhase MatchPhase = EPBMatchPhase::Lobby;

	/** Server world time at which the current phase ends (0 = untimed). */
	UPROPERTY(Replicated, Transient)
	double PhaseEndServerTime = 0.0;

	UPROPERTY(Replicated, Transient)
	int32 CurrentHoleIndex = 0;

	/** Active host-configured rules for this match (set at match start). */
	UPROPERTY(Replicated, Transient)
	FPBMatchSettings MatchSettings;
};
