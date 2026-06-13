// Copyright Epic Games, Inc. All Rights Reserved.

#include "PBCameraSubsystem.h"

#include "Camera/PBCameraRig.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

void UPBCameraSubsystem::RefreshRigs()
{
	Rigs.Reset();
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<APBCameraRig> It(World); It; ++It)
		{
			Rigs.Add(*It);
		}
	}
	Rigs.Sort([](const APBCameraRig& A, const APBCameraRig& B)
	{
		return A.Priority < B.Priority;
	});
	bCollected = true;
	CurrentIndex = Rigs.Num() > 0 ? 0 : INDEX_NONE;
}

void UPBCameraSubsystem::EnsureRigsCollected()
{
	if (!bCollected)
	{
		RefreshRigs();
	}
}

void UPBCameraSubsystem::ActivateFirstRig(APlayerController* PC)
{
	EnsureRigsCollected();
	if (!PC || Rigs.Num() == 0)
	{
		return;
	}
	CurrentIndex = 0;
	PC->SetViewTargetWithBlend(Rigs[CurrentIndex], 0.f);
}

void UPBCameraSubsystem::CycleCamera(APlayerController* PC)
{
	EnsureRigsCollected();
	if (!PC || Rigs.Num() == 0)
	{
		return;
	}
	CurrentIndex = (CurrentIndex + 1) % Rigs.Num();
	PC->SetViewTargetWithBlend(Rigs[CurrentIndex], BlendTime);
}
