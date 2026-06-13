// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PBUIPlaceholder.generated.h"

/**
 * Placeholder that locks in the UI subsystem folder (CONVENTIONS §1).
 * The real UMG widgets / HUD land across the UI-touching phases (plans/09-10).
 * Delete this once a real class lives in this folder.
 */
UCLASS()
class PUTTBATTLE_API UPBUIPlaceholder : public UObject
{
	GENERATED_BODY()
};
