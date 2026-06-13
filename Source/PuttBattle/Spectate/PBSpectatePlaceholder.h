// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PBSpectatePlaceholder.generated.h"

/**
 * Placeholder that locks in the Spectate subsystem folder (CONVENTIONS §1).
 * The real spectator system lands in plans/08-spectator-system.
 * Delete this once a real class lives in this folder.
 */
UCLASS()
class PUTTBATTLE_API UPBSpectatePlaceholder : public UObject
{
	GENERATED_BODY()
};
