// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PBPowerupsPlaceholder.generated.h"

/**
 * Placeholder that locks in the Powerups subsystem folder (CONVENTIONS §1).
 * The real custom powerup framework (UPBPowerupEffect etc.) lands in plans/05-powerup-framework.
 * Delete this once a real class lives in this folder.
 */
UCLASS()
class PUTTBATTLE_API UPBPowerupsPlaceholder : public UObject
{
	GENERATED_BODY()
};
