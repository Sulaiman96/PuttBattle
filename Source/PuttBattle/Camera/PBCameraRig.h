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

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PB|Camera")
	TObjectPtr<UCameraComponent> Camera;

	/** Lower numbers come first when cycling. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Camera")
	int32 Priority = 0;

	// --- Auto-orbit overview (cam 3) ---------------------------------------
	// When enabled, this rig ignores its authored rotation and slowly sweeps a
	// 360° arc around the point it is placed at, looking inward — the shared
	// "whole map" overview. It is player-independent (not parented to any ball)
	// and time-driven, so every client sees the same sweep with no replication
	// (D4: cameras are presentation only). Place ONE in the level, at the point
	// you want it to circle (usually the centre of the hole), to enable cam 3.

	/** Turn this rig into the slow auto-orbiting overview camera. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Camera|Orbit")
	bool bAutoOrbit = false;

	/** Sweep speed (degrees/second) — keep it small for a calm overview. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Camera|Orbit", meta = (EditCondition = "bAutoOrbit"))
	float OrbitDegreesPerSecond = 6.f;

	/** Horizontal distance (cm) the camera sits from the pivot it circles. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Camera|Orbit", meta = (EditCondition = "bAutoOrbit"))
	float OrbitRadius = 2200.f;

	/** Height (cm) the camera floats above the pivot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Camera|Orbit", meta = (EditCondition = "bAutoOrbit"))
	float OrbitHeight = 1600.f;

	/** Field of view (deg) while orbiting — wider than a framing rig so the whole map fits. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Camera|Orbit", meta = (EditCondition = "bAutoOrbit"))
	float OrbitFieldOfView = 55.f;

protected:
	virtual void BeginPlay() override;

private:
	/** The placed location, captured at BeginPlay — the fixed point the camera circles
	 *  (GetActorLocation() can't be the pivot because Tick moves the camera root). */
	FVector OrbitPivot = FVector::ZeroVector;
};
