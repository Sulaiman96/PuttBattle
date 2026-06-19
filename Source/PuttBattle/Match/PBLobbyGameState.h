// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "PBMatchTypes.h"
#include "PBLobbyGameState.generated.h"

/**
 * The lobby's replicated facts (plans/03 T3.4): the host-configured match
 * settings everyone previews, and the ready-up tally. Seats are just the
 * inherited PlayerArray; ready state lives on each APBPlayerState. The host
 * (server) is the only writer of the settings (host-only settings panel).
 */
UCLASS()
class PUTTBATTLE_API APBLobbyGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "PB|Lobby")
	const FPBMatchSettings& GetMatchSettings() const { return MatchSettings; }

	/** True when there are at least 2 players (D16) and all of them are ready. */
	UFUNCTION(BlueprintPure, Category = "PB|Lobby")
	bool AreAllPlayersReady() const;

	/** Number of connected players (seats taken). */
	UFUNCTION(BlueprintPure, Category = "PB|Lobby")
	int32 GetNumPlayers() const { return PlayerArray.Num(); }

	/** Host writes the settings here (server only). */
	void SetMatchSettings(const FPBMatchSettings& NewSettings);

private:
	UPROPERTY(Replicated, Transient)
	FPBMatchSettings MatchSettings;
};
