// Copyright Epic Games, Inc. All Rights Reserved.

#include "PBMatchGameState.h"

#include "Match/PBPlayerState.h"
#include "Net/UnrealNetwork.h"

void APBMatchGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APBMatchGameState, MatchPhase);
	DOREPLIFETIME(APBMatchGameState, PhaseEndServerTime);
	DOREPLIFETIME(APBMatchGameState, CurrentHoleIndex);
	DOREPLIFETIME(APBMatchGameState, MatchSettings);
}

float APBMatchGameState::GetPhaseTimeRemaining() const
{
	if (PhaseEndServerTime <= 0.0)
	{
		return 0.f;
	}
	// GetServerWorldTimeSeconds() returns the replicated server clock (double in
	// 5.7), so the same subtraction is valid on the server and every client.
	return static_cast<float>(FMath::Max(0.0, PhaseEndServerTime - GetServerWorldTimeSeconds()));
}

bool APBMatchGameState::AreAllPlayersFinished() const
{
	bool bAnyPlayer = false;
	for (const TObjectPtr<APlayerState>& PS : PlayerArray)
	{
		const APBPlayerState* PBPS = Cast<APBPlayerState>(PS);
		if (!PBPS)
		{
			continue; // ignore non-PB / transient states
		}
		bAnyPlayer = true;
		if (!PBPS->IsFinished())
		{
			return false;
		}
	}
	return bAnyPlayer; // all finished, and there was at least one
}

void APBMatchGameState::SetMatchPhase(EPBMatchPhase NewPhase, float Duration)
{
	if (!HasAuthority())
	{
		return;
	}
	MatchPhase = NewPhase;
	PhaseEndServerTime = Duration > 0.f ? GetServerWorldTimeSeconds() + Duration : 0.0;
	// Server has no OnRep — notify host-side listeners directly.
	OnMatchPhaseChanged.Broadcast(MatchPhase);
}

void APBMatchGameState::SetCurrentHoleIndex(int32 NewIndex)
{
	if (HasAuthority())
	{
		CurrentHoleIndex = NewIndex;
	}
}

void APBMatchGameState::SetMatchSettings(const FPBMatchSettings& NewSettings)
{
	if (HasAuthority())
	{
		MatchSettings = NewSettings;
	}
}

void APBMatchGameState::OnRep_MatchPhase()
{
	OnMatchPhaseChanged.Broadcast(MatchPhase);
}
