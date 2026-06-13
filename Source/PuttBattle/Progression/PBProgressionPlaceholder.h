// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PBProgressionPlaceholder.generated.h"

/**
 * Placeholder that locks in the Progression subsystem folder (CONVENTIONS §1).
 * The real progression/cosmetics/save system lands in plans/09-progression-cosmetics.
 * Delete this once a real class lives in this folder.
 */
UCLASS()
class PUTTBATTLE_API UPBProgressionPlaceholder : public UObject
{
	GENERATED_BODY()
};
