// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PBCheckpointActor.generated.h"

class APBBallPawn;
class USphereComponent;
class UPrimitiveComponent;

/**
 * A per-player respawn anchor (plans/02 T2.3, D7). Activates on proximity and is
 * ordered by CheckpointIndex; the GameMode records each ball's highest-activated
 * checkpoint and respawns there on a hazard / fall. Activation FX is fired only
 * for the activating local player now; Phase 3 adds replicated per-player
 * filtering. Tracking is keyed per ball here, per PlayerState later — the shape
 * is the same.
 */
UCLASS()
class PUTTBATTLE_API APBCheckpointActor : public AActor
{
	GENERATED_BODY()

public:
	APBCheckpointActor();

	/** Ordering along the hole; the latest (highest) activated is the respawn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Course")
	int32 CheckpointIndex = 0;

	/** Where a ball reappears when respawning here (lifted onto the floor). */
	UFUNCTION(BlueprintPure, Category = "PB|Course")
	FTransform GetRespawnTransform() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Course")
	float RespawnZOffset = 10.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PB|Course")
	TObjectPtr<USphereComponent> ActivationZone;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep);

	/** Subtle FX shown only to the activating local player (P1: local check). */
	UFUNCTION(BlueprintImplementableEvent, Category = "PB|Course")
	void OnActivatedForLocalPlayer();

private:
	/** Balls that have already activated this checkpoint (avoid refiring FX). */
	UPROPERTY(Transient)
	TSet<TObjectPtr<APBBallPawn>> ActivatedBalls;
};
