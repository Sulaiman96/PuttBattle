// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PBCameraRig.generated.h"

class UCameraComponent;

/**
 * A fixed per-hole camera position (D4: 2–4 rigs per variant, player cycles).
 * Placed in the level and pointed by hand; the BP subclass / instance carries
 * the isometric framing (default ~52° pitch, FOV 35 for a compressed diorama
 * look). Purely presentation — never replicated.
 */
UCLASS()
class PUTTBATTLE_API APBCameraRig : public AActor
{
	GENERATED_BODY()

public:
	APBCameraRig();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PB|Camera")
	TObjectPtr<UCameraComponent> Camera;

	/** Lower numbers come first when cycling. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Camera")
	int32 Priority = 0;
};
