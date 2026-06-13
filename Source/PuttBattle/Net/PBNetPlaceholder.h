// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PBNetPlaceholder.generated.h"

/**
 * Placeholder that locks in the Net subsystem folder (CONVENTIONS §1).
 * The real replication/session plumbing lands in plans/03-multiplayer-core.
 * Delete this once a real class lives in this folder.
 */
UCLASS()
class PUTTBATTLE_API UPBNetPlaceholder : public UObject
{
	GENERATED_BODY()
};
