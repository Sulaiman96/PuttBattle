// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PBCameraSubsystem.generated.h"

class APBCameraRig;
class APlayerController;

/**
 * Client-only camera director (D4 / plans/01 T1.4). Collects the hole's camera
 * rigs sorted by Priority and cycles the player's view target with a short blend
 * on the IA_CycleCamera press. Holds no game truth — never replicated. Aim stays
 * correct after a cycle because UPBShotComponent re-derives camera-relative aim
 * every frame from the live camera rotation (D2), not from a cached basis.
 */
UCLASS()
class PUTTBATTLE_API UPBCameraSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Advance to the next rig (wraps) and blend the PC's view target to it. */
	UFUNCTION(BlueprintCallable, Category = "PB|Camera")
	void CycleCamera(APlayerController* PC);

	/** Snap the PC to the first (lowest-priority) rig with no blend. */
	UFUNCTION(BlueprintCallable, Category = "PB|Camera")
	void ActivateFirstRig(APlayerController* PC);

	/** Re-scan the level for camera rigs (call after a hole loads). */
	UFUNCTION(BlueprintCallable, Category = "PB|Camera")
	void RefreshRigs();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Camera")
	float BlendTime = 0.3f;

private:
	void EnsureRigsCollected();

	UPROPERTY(Transient)
	TArray<TObjectPtr<APBCameraRig>> Rigs;

	int32 CurrentIndex = INDEX_NONE;
	bool bCollected = false;
};
