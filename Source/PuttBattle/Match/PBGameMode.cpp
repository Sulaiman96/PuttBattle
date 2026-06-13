// Copyright Epic Games, Inc. All Rights Reserved.

#include "PBGameMode.h"

#include "Ball/PBBallPawn.h"
#include "Course/PBCheckpointActor.h"
#include "Course/PBTeePad.h"
#include "EngineUtils.h"
#include "Match/PBPlayerController.h"
#include "Shot/PBShotComponent.h"

APBGameMode::APBGameMode()
{
	DefaultPawnClass = APBBallPawn::StaticClass();
	PlayerControllerClass = APBPlayerController::StaticClass();
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
}

void APBGameMode::ActivateCheckpoint(APBBallPawn* Ball, APBCheckpointActor* Checkpoint)
{
	if (!Ball || !Checkpoint)
	{
		return;
	}
	const TObjectPtr<APBCheckpointActor>* Existing = LatestCheckpoint.Find(Ball);
	if (!Existing || !*Existing || Checkpoint->CheckpointIndex >= (*Existing)->CheckpointIndex)
	{
		LatestCheckpoint.Add(Ball, Checkpoint);
	}
}

void APBGameMode::RespawnAtCheckpoint(APBBallPawn* Ball)
{
	if (!Ball)
	{
		return;
	}

	FTransform Respawn;
	if (const TObjectPtr<APBCheckpointActor>* Cp = LatestCheckpoint.Find(Ball); Cp && *Cp)
	{
		Respawn = (*Cp)->GetRespawnTransform();
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
