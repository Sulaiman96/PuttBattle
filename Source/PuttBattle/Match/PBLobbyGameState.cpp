// Copyright Epic Games, Inc. All Rights Reserved.

#include "PBLobbyGameState.h"

#include "Match/PBPlayerState.h"
#include "Net/UnrealNetwork.h"

void APBLobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APBLobbyGameState, MatchSettings);
}

bool APBLobbyGameState::AreAllPlayersReady() const
{
	int32 NumPlayers = 0;
	for (const TObjectPtr<APlayerState>& PS : PlayerArray)
	{
		const APBPlayerState* PBPS = Cast<APBPlayerState>(PS);
		if (!PBPS)
		{
			continue;
		}
		++NumPlayers;
		if (!PBPS->IsReady())
		{
			return false;
		}
	}
	return NumPlayers >= 2; // lobby minimum (D16)
}

void APBLobbyGameState::SetMatchSettings(const FPBMatchSettings& NewSettings)
{
	if (HasAuthority())
	{
		MatchSettings = NewSettings;
	}
}
