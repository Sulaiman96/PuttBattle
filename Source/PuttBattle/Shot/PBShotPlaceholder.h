// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PBShotPlaceholder.generated.h"

/**
 * Placeholder that locks in the Shot subsystem folder (CONVENTIONS §1).
 * The real UPBShotComponent / FPBShotRequest land in plans/01-ball-and-shot.
 * Delete this once a real class lives in this folder.
 */
UCLASS()
class PUTTBATTLE_API UPBShotPlaceholder : public UObject
{
	GENERATED_BODY()
};
