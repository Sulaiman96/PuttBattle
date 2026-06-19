// Copyright Epic Games, Inc. All Rights Reserved.

#include "PBPlayerState.h"

#include "Net/UnrealNetwork.h"

APBPlayerState::APBPlayerState()
{
	// PlayerState already replicates; nothing physics-y here. Keep the default
	// net update cadence — these are low-frequency, event-driven fields.
}

void APBPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APBPlayerState, Strokes);
	DOREPLIFETIME(APBPlayerState, TotalScore);
	DOREPLIFETIME(APBPlayerState, CheckpointIndex);
	DOREPLIFETIME(APBPlayerState, bFinished);
	DOREPLIFETIME(APBPlayerState, FinishOrder);
	DOREPLIFETIME(APBPlayerState, FinishServerTime);
	DOREPLIFETIME(APBPlayerState, bReady);
}

void APBPlayerState::CopyProperties(APlayerState* NewPlayerState)
{
	Super::CopyProperties(NewPlayerState);

	// Seamless-travel carrier (lobby→match, and hole→hole if we ever travel
	// between holes): `this` is the OLD state, the argument is the NEW one.
	// TotalScore must persist across the hop; the per-hole fields are copied too
	// but the match GameMode re-zeroes them at each HoleIntro, so a fresh hole
	// always starts clean.
	if (APBPlayerState* PB = Cast<APBPlayerState>(NewPlayerState))
	{
		PB->Strokes = Strokes;
		PB->TotalScore = TotalScore;
		PB->CheckpointIndex = CheckpointIndex;
		PB->bFinished = bFinished;
		PB->FinishOrder = FinishOrder;
		PB->FinishServerTime = FinishServerTime;
	}
}

void APBPlayerState::AddStroke()
{
	if (!HasAuthority())
	{
		return;
	}
	++Strokes;
	// The listen-server host is the server, so OnRep won't fire for its own
	// PlayerState — broadcast here so the host's HUD updates too.
	OnStrokesChanged.Broadcast(Strokes);
}

void APBPlayerState::ResetForNewHole()
{
	if (!HasAuthority())
	{
		return;
	}
	const bool bWasFinished = bFinished;
	Strokes = 0;
	CheckpointIndex = INDEX_NONE;
	bFinished = false;
	FinishOrder = INDEX_NONE;
	FinishServerTime = 0.0;
	// Server has no OnRep — fire the presentation hooks directly so the listen-server
	// host's UI/state clears too (clients get them via OnRep_Strokes/OnRep_Finished).
	OnStrokesChanged.Broadcast(Strokes);
	if (bWasFinished)
	{
		OnFinishedChanged(false);
	}
}

void APBPlayerState::SetCheckpointIndex(int32 NewIndex)
{
	if (!HasAuthority())
	{
		return;
	}
	// Never regress: keep the highest activated checkpoint (D7).
	if (NewIndex > CheckpointIndex)
	{
		CheckpointIndex = NewIndex;
	}
}

void APBPlayerState::MarkFinished(int32 Order, double ServerTime)
{
	if (!HasAuthority() || bFinished)
	{
		return;
	}
	bFinished = true;
	FinishOrder = Order;
	FinishServerTime = ServerTime;
	OnFinishedChanged(true); // host-side presentation hook (clients get it via OnRep)
}

void APBPlayerState::SetReady(bool bInReady)
{
	if (HasAuthority())
	{
		bReady = bInReady;
	}
}

void APBPlayerState::OnRep_Strokes()
{
	OnStrokesChanged.Broadcast(Strokes);
}

void APBPlayerState::OnRep_Finished()
{
	OnFinishedChanged(bFinished);
}
