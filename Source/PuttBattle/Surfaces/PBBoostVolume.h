// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PBBoostVolume.generated.h"

class APBBallPawn;
class UArrowComponent;
class UBoxComponent;
class UPrimitiveComponent;

/**
 * A placed boost pad (plans/02 T2.2). Boost is the one surface that needs an
 * actor rather than pure asset data, because it imparts directional force: while
 * a ball overlaps, it's accelerated along the arrow's forward axis. Generic
 * enough that pads are placed content — direction + strength are per-instance.
 */
UCLASS()
class PUTTBATTLE_API APBBoostVolume : public AActor
{
	GENERATED_BODY()

public:
	APBBoostVolume();

	virtual void Tick(float DeltaSeconds) override;

	/** Acceleration imparted along the arrow forward while overlapped (cm/s²). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Surface|Boost", meta = (ClampMin = "0.0"))
	float BoostAcceleration = 5000.f;

	/** Speed cap (cm/s) along the boost axis — stop pushing past this so the pad is
	 *  self-bounding and tunable independent of the ball's MaxSpeed. <= 0 = no cap. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Surface|Boost", meta = (ClampMin = "0.0"))
	float MaxBoostSpeed = 2500.f;

	/** Trigger box half-extents (cm). Length along travel scales total impulse. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Surface|Boost")
	FVector ZoneExtent = FVector(50.f, 50.f, 20.f);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PB|Surface|Boost")
	TObjectPtr<UBoxComponent> BoostZone;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PB|Surface|Boost")
	TObjectPtr<UArrowComponent> DirectionArrow;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep);

	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

private:
	UPROPERTY(Transient)
	TSet<TObjectPtr<APBBallPawn>> OverlappingBalls;
};
