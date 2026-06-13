// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "PBSurfaceSamplerComponent.generated.h"

class APBBallPawn;
class UPBSurfaceDefinition;

/**
 * Reads the surface under the ball and applies a controllable rolling-drag force
 * (plans/02 T2.1). Chaos friction alone won't give readable, tunable roll feel,
 * so we add our own deceleration opposing horizontal velocity, scaled by the
 * active surface's RollDragMultiplier. The active surface is the top of the
 * UPBSurfaceSubsystem override stack if one is pushed (Ice Age), else whatever
 * the down-trace lands on. Drag is only applied while grounded — airborne ramp
 * shots are untouched (D3).
 */
UCLASS(ClassGroup = (PB), meta = (BlueprintSpawnableComponent))
class PUTTBATTLE_API UPBSurfaceSamplerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPBSurfaceSamplerComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** The surface currently governing the ball (override or contact). */
	UFUNCTION(BlueprintPure, Category = "PB|Surface")
	UPBSurfaceDefinition* GetActiveDefinition() const { return ActiveDefinition; }

	UFUNCTION(BlueprintPure, Category = "PB|Surface")
	FGameplayTag GetActiveSurfaceTag() const;

	/**
	 * Pure roll deceleration model shared with the automation test (cm/s²):
	 * a = Speed × RollDragMultiplier × Coefficient. Higher multiplier → shorter
	 * roll. Extracted so roll-distance ordering can be verified deterministically.
	 */
	static float ComputeRollDeceleration(float Speed, float RollDragMultiplier, float Coefficient);

	/** Used when no surface resolves (bare floor with no PB physical material). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Surface")
	TObjectPtr<UPBSurfaceDefinition> FallbackDefinition;

	/** Global tuning for the custom roll-drag force (mirrored by pb.Surface.DragCoefficient). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Surface|Feel", meta = (ClampMin = "0.0"))
	float RollDragCoefficient = 1.5f;

	/** How far below the ball to look for a floor (cm beyond the sphere radius). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Surface|Feel", meta = (ClampMin = "0.0"))
	float GroundTraceSlack = 8.f;

	/** Don't bother applying drag below this planar speed (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Surface|Feel", meta = (ClampMin = "0.0"))
	float MinDragSpeed = 2.f;

protected:
	virtual void BeginPlay() override;

private:
	APBBallPawn* GetBall() const;

	/** Down-trace for the contact surface; returns its definition or null. */
	UPBSurfaceDefinition* SampleContactDefinition() const;

	UPROPERTY(Transient)
	TObjectPtr<UPBSurfaceDefinition> ActiveDefinition;
};
