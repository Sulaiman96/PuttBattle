// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PBCameraPlaceholder.generated.h"

/**
 * Placeholder that locks in the Camera subsystem folder (CONVENTIONS §1).
 * The real client-authoritative camera rig lands in the ball/shot & spectate phases.
 * Delete this once a real class lives in this folder.
 */
UCLASS()
class PUTTBATTLE_API UPBCameraPlaceholder : public UObject
{
	GENERATED_BODY()
};
