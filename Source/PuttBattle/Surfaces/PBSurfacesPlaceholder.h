// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PBSurfacesPlaceholder.generated.h"

/**
 * Placeholder that locks in the Surfaces subsystem folder (CONVENTIONS §1).
 * The real surface definitions / physical materials land in plans/02-surfaces-hazards-checkpoints.
 * Delete this once a real class lives in this folder.
 */
UCLASS()
class PUTTBATTLE_API UPBSurfacesPlaceholder : public UObject
{
	GENERATED_BODY()
};
