// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PBMatchTypes.h"
#include "PBLobbyGameMode.generated.h"

/**
 * The pre-match lobby referee (plans/03 T3.4). Seats players (2–6), tracks the
 * host (first to log in on the listen server), accepts host-only settings edits,
 * and — once everyone is ready — seamless-travels the whole party into the match
 * map. The chosen FPBMatchSettings are handed to the SessionSubsystem so the
 * match GameMode picks them up after the hop (the lobby GameState doesn't survive
 * travel).
 *
 * No ball in the lobby (DefaultPawnClass cleared) — players are UI-only until the
 * match starts.
 */
UCLASS()
class PUTTBATTLE_API APBLobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	APBLobbyGameMode();

	virtual void PostLogin(APlayerController* NewPlayer) override;

	/** Host-only: write the previewed match settings to the lobby GameState. */
	void HostSetMatchSettings(APlayerController* By, const FPBMatchSettings& NewSettings);

	/** Host-only: if everyone is ready, carry settings forward and travel to the match. */
	void RequestStartMatch(APlayerController* By);

	/** Map the match begins on (Phase 3: the graybox test hole; variants are Phase 7). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "PB|Lobby")
	FString MatchMapURL = TEXT("/Game/Maps/Holes/H_Test/V_A");

	/** GameMode forced on the match map via ?game=, so the match runs APBMatchGameMode
	 *  while V_A's own World Settings stay on the offline referee (feel-test friendly). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "PB|Lobby")
	FString MatchGameModeURL = TEXT("/Script/PuttBattle.PBMatchGameMode");

protected:
	/** The listen-server host (first PostLogin) — the only client allowed to configure/start. */
	UPROPERTY(Transient)
	TObjectPtr<APlayerController> HostController = nullptr;
};
