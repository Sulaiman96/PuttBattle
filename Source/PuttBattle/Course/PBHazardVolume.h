// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "PBHazardVolume.generated.h"

class APBBallPawn;
class UBoxComponent;
class UPrimitiveComponent;

/**
 * Water / void hazard (plans/02 T2.3, D7). A ball that enters is respawned at its
 * latest checkpoint (tee if none) with zero velocity and no penalty stroke. The
 * KillZ path routes through the same GameMode::RespawnAtCheckpoint, so falling
 * off a ramp behaves identically. HazardTag picks the cosmetic variant
 * (Hazard.Water splash vs Hazard.Void) — behaviour is the same reset.
 */
UCLASS()
class PUTTBATTLE_API APBHazardVolume : public AActor
{
	GENERATED_BODY()

public:
	APBHazardVolume();

	/** Hazard.Water or Hazard.Void (CONVENTIONS §5). Drives the splash variant. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Course")
	FGameplayTag HazardTag;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PB|Course")
	TObjectPtr<UBoxComponent> HazardZone;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep);

	/** Cosmetic splash/feedback at the entry point (local). */
	UFUNCTION(BlueprintImplementableEvent, Category = "PB|Course")
	void OnBallEntered(const FVector& EntryLocation);
};
