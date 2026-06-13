// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PBMatchPlaceholder.generated.h"

/**
 * Placeholder that locks in the Match subsystem folder (CONVENTIONS §1).
 * The real GameMode/GameState match-flow & scoring land in plans/04-match-flow-timers-scoring.
 * Delete this once a real class lives in this folder.
 */
UCLASS()
class PUTTBATTLE_API UPBMatchPlaceholder : public UObject
{
	GENERATED_BODY()
};
