// Copyright Epic Games, Inc. All Rights Reserved.

#include "PBGameMode.h"

#include "Ball/PBBallPawn.h"
#include "Course/PBCheckpointActor.h"
#include "Course/PBTeePad.h"
#include "EngineUtils.h"
#include "Match/PBPlayerController.h"
#include "Match/PBPlayerState.h"
#include "Shot/PBShotComponent.h"

APBGameMode::APBGameMode()
{
	DefaultPawnClass = APBBallPawn::StaticClass();
	PlayerControllerClass = APBPlayerController::StaticClass();
	PlayerStateClass = APBPlayerState::StaticClass();
}

void APBGameMode::RestartHole(APlayerController* PC)
{
	if (!PC)
	{
		return;
	}
	APBBallPawn* Ball = Cast<APBBallPawn>(PC->GetPawn());
	if (!Ball)
	{
		return;
	}

	const APBTeePad* Tee = FindTeePad();
	const FTransform Spawn = Tee ? Tee->GetSpawnTransform() : Ball->GetActorTransform();
	Ball->TeleportToAndStop(Spawn.GetLocation(), Spawn.Rotator());

	if (UPBShotComponent* Shot = Ball->GetShotComponent())
	{
		Shot->ResetForNewHole();
	}

	// Strokes/checkpoint/finish are authoritative per-hole state on the PlayerState.
	if (APBPlayerState* PS = PC->GetPlayerState<APBPlayerState>())
	{
		PS->ResetForNewHole();
	}
}

void APBGameMode::ActivateCheckpoint(APBBallPawn* Ball, APBCheckpointActor* Checkpoint)
{
	if (!Ball || !Checkpoint)
	{
		return;
	}
	// Per-player, server-authoritative; SetCheckpointIndex keeps only the highest (D7).
	if (APBPlayerState* PS = Ball->GetPlayerState<APBPlayerState>())
	{
		PS->SetCheckpointIndex(Checkpoint->CheckpointIndex);
	}
}

void APBGameMode::RespawnAtCheckpoint(APBBallPawn* Ball)
{
	if (!Ball)
	{
		return;
	}

	int32 Index = INDEX_NONE;
	if (const APBPlayerState* PS = Ball->GetPlayerState<APBPlayerState>())
	{
		Index = PS->GetCheckpointIndex();
	}

	FTransform Respawn;
	if (const APBCheckpointActor* Cp = FindCheckpointByIndex(Index))
	{
		Respawn = Cp->GetRespawnTransform();
	}
	else if (const APBTeePad* Tee = FindTeePad())
	{
		Respawn = Tee->GetSpawnTransform();
	}
	else
	{
		Respawn = Ball->GetActorTransform();
	}

	// Zero velocity, no stroke change (D7).
	Ball->TeleportToAndStop(Respawn.GetLocation(), Respawn.Rotator());
}

APBTeePad* APBGameMode::FindTeePad() const
{
	if (const UWorld* World = GetWorld())
	{
		for (TActorIterator<APBTeePad> It(World); It; ++It)
		{
			return *It;
		}
	}
	return nullptr;
}

APBCheckpointActor* APBGameMode::FindCheckpointByIndex(int32 Index) const
{
	if (Index == INDEX_NONE)
	{
		return nullptr; // no checkpoint activated → caller falls back to the tee
	}
	if (const UWorld* World = GetWorld())
	{
		for (TActorIterator<APBCheckpointActor> It(World); It; ++It)
		{
			if (It->CheckpointIndex == Index)
			{
				return *It;
			}
		}
	}
	return nullptr;
}
