// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PBCupActor.generated.h"

class APBBallPawn;
class UPrimitiveComponent;
class USphereComponent;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPBBallSunk, APBBallPawn*, Ball);

/**
 * The hole. A ball sinks iff it overlaps the catch zone AND its speed is below
 * SinkSpeedThreshold AND it isn't Ballooned (ScaleMultiplier < BalloonedMaxScale,
 * DF6) — a fast or inflated ball rolls over the lip. The catch is re-evaluated
 * every tick, not just on enter, so a ball that rolls in fast and settles still
 * drops. Sink authority is trivial in Phase 1 (offline); Phase 3 gates it on the
 * server, but the validity rule stays here.
 */
UCLASS()
class PUTTBATTLE_API APBCupActor : public AActor
{
	GENERATED_BODY()

public:
	APBCupActor();

	virtual void Tick(float DeltaSeconds) override;

	/** Broadcast once when a ball is accepted (HUD "SUNK!" toast, scoring later). */
	UPROPERTY(BlueprintAssignable, Category = "PB|Course")
	FPBBallSunk OnBallSunk;

	/** Max ball speed (cm/s) that still drops instead of rolling over (DF6). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Course", meta = (ClampMin = "0.0"))
	float SinkSpeedThreshold = 120.f;

	/** A ball at or above this scale is too big to fit the cup (Balloon, DF6). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Course", meta = (ClampMin = "1.0"))
	float BalloonedMaxScale = 1.3f;

	/** Catch-zone radius (cm) — how close to centre a ball must pass to drop. The
	 *  third cup-difficulty knob beside speed + scale. Applied at BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Course", meta = (ClampMin = "1.0"))
	float CatchRadius = 12.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PB|Course")
	TObjectPtr<USphereComponent> CatchZone;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PB|Course")
	TObjectPtr<UStaticMeshComponent> CupMesh;

protected:
	virtual void BeginPlay() override;

	/** Has this ball already been accepted (avoid double-broadcast). */
	bool IsBallSunk(const APBBallPawn* Ball) const;

	void AcceptBall(APBBallPawn* Ball);

private:
	UPROPERTY(Transient)
	TSet<TObjectPtr<APBBallPawn>> SunkBalls;
};
