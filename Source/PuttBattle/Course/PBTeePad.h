// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerStart.h"
#include "PBTeePad.generated.h"

/**
 * The hole's spawn transform. Subclasses APlayerStart so the GameMode's default
 * spawn logic uses it for free, while GetSpawnTransform() gives the respawn /
 * restart path a single source of truth (lifted slightly so the ball drops in).
 */
UCLASS()
class PUTTBATTLE_API APBTeePad : public APlayerStart
{
	GENERATED_BODY()

public:
	APBTeePad(const FObjectInitializer& ObjectInitializer);

	/** Spawn point for the ball, lifted by SpawnZOffset above the pad. */
	UFUNCTION(BlueprintPure, Category = "PB|Course")
	FTransform GetSpawnTransform() const;

	/** Height above the pad to spawn the ball so it settles onto the floor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Course")
	float SpawnZOffset = 10.f;
};
