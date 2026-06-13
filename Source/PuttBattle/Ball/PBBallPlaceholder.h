// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PBBallPlaceholder.generated.h"

/**
 * Placeholder that locks in the Ball subsystem folder (CONVENTIONS §1).
 * The real ball pawn (APBBallPawn) and attributes land in plans/01-ball-and-shot.
 * Delete this once a real class lives in this folder.
 */
UCLASS()
class PUTTBATTLE_API UPBBallPlaceholder : public UObject
{
	GENERATED_BODY()
};
