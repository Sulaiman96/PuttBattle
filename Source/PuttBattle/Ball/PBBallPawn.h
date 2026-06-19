// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "PBBallAttributes.h"
#include "PBBallPawn.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UPBShotComponent;
class UPBSurfaceSamplerComponent;

/**
 * The player IS the ball (D1). A physics-simulating sphere with the attributes
 * pattern installed. C++ owns physics/collision; a BP subclass (BP_BallPawn)
 * wires the cosmetic mesh + power curve and carries the tuned feel values so
 * iteration never needs a recompile (D21, CONVENTIONS §1 feel-tuning rule).
 *
 * Every feel constant is reachable three ways: the UPROPERTYs below, the BP
 * subclass defaults, and the `pb.Ball.*` console variables for live in-PIE
 * tuning (UA-9). A CVar set to a non-negative value overrides the property.
 */
UCLASS()
class PUTTBATTLE_API APBBallPawn : public APawn
{
	GENERATED_BODY()

public:
	APBBallPawn();

	virtual void Tick(float DeltaSeconds) override;
	/** KillZ / falling off the map routes to the checkpoint respawn path (D7), not destruction. */
	virtual void FellOutOfWorld(const UDamageType& DmgType) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	/** Physics sphere — root, simulates, profile PB_BallProfile, CCD on. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PB|Ball")
	TObjectPtr<USphereComponent> CollisionSphere;

	/** Cosmetic mesh, NoCollision. Wired in BP_BallPawn. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PB|Ball")
	TObjectPtr<UStaticMeshComponent> BallMesh;

	/** Drag-aim-release shot loop (Phase 1). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PB|Ball")
	TObjectPtr<UPBShotComponent> ShotComponent;

	/** Per-surface roll behaviour sampler (Phase 2). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PB|Ball")
	TObjectPtr<UPBSurfaceSamplerComponent> SurfaceSampler;

	// --- Feel constants (BP-tunable; mirrored by pb.Ball.* CVars) ----------

	/**
	 * Collision sphere radius in cm at ScaleMultiplier 1 — the ball's physical
	 * size, core feel (roll character, what gaps it fits, cup-fit margin). Applied
	 * at BeginPlay + on editor edit, not per-frame (SetSphereRadius rebuilds bounds
	 * and is expensive). Change it on BP_BallPawn; no live CVar by design.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Ball|Feel", meta = (ClampMin = "0.1"))
	float BaseRadiusCm = 6.f;

	/** Base mass in kg at ScaleMultiplier 1. Mass scales with volume (∝ scale³). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Ball|Feel", meta = (ClampMin = "0.01"))
	float BaseMassKg = 0.045f;

	/** Linear damping — how quickly the ball bleeds rolling speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Ball|Feel", meta = (ClampMin = "0.0"))
	float LinearDamping = 0.05f;

	/** Angular damping — how quickly the ball stops spinning. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Ball|Feel", meta = (ClampMin = "0.0"))
	float AngularDamping = 0.6f;

	/** Hard ceiling on linear speed (cm/s) so a max-power shot stays sane. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Ball|Feel", meta = (ClampMin = "0.0"))
	float MaxSpeed = 3000.f;

	/** Impulse delivered by a full-power shot before PowerMultiplier (cm/s·kg). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Ball|Feel", meta = (ClampMin = "0.0"))
	float MaxImpulse = 90.f;

	// --- Networking (replication cadence) ----------------------------------
	// The ball replicates its physics state server→clients (Predictive
	// Interpolation, set in BeginPlay). To keep bandwidth sane we replicate fast
	// while the ball moves and slowly once it rests — the "dormancy-ish" toggle
	// from plans/03 T3.1, done as a net-rate throttle rather than true dormancy
	// (dormancy can freeze a ball mid-motion if a non-shot force — boost, gust —
	// moves a dormant body). Server-side only.

	/** Net update rate (Hz) while the ball is moving. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Ball|Net", meta = (ClampMin = "1.0"))
	float MovingNetUpdateHz = 30.f;

	/** Net update rate (Hz) once the ball is at rest. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Ball|Net", meta = (ClampMin = "0.5"))
	float RestNetUpdateHz = 5.f;

	/** Speed (cm/s) below which the ball counts as at-rest for the net-rate throttle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Ball|Net", meta = (ClampMin = "0.0"))
	float NetRestSpeedThreshold = 5.f;

	// --- Attributes --------------------------------------------------------

	/** The one channel for every gameplay modifier (CONVENTIONS §3). */
	UPROPERTY(ReplicatedUsing = OnRep_Attributes, EditAnywhere, BlueprintReadWrite, Category = "PB|Ball|Attributes")
	FPBBallAttributes Attributes;

	/** Server-side mutation entry point: set attributes then re-apply. */
	void SetAttributes(const FPBBallAttributes& NewAttributes);

	/** Applies Attributes to the pawn in one place (scale → mass, etc.). */
	UFUNCTION(BlueprintCallable, Category = "PB|Ball")
	void ApplyAttributes();

	/** Effective MaxImpulse after the pb.Ball.MaxImpulse CVar override. */
	float GetEffectiveMaxImpulse() const;

	/** Current planar speed (cm/s), ignoring vertical motion. */
	UFUNCTION(BlueprintPure, Category = "PB|Ball")
	float GetPlanarSpeed() const;

	/** True planar+vertical speed (cm/s). */
	UFUNCTION(BlueprintPure, Category = "PB|Ball")
	float GetSpeed() const;

	/** Zero linear + angular velocity (respawn / anchor / timeout). */
	UFUNCTION(BlueprintCallable, Category = "PB|Ball")
	void StopMotion();

	/** Teleport to a transform and zero velocity (respawn path, Phase 2). */
	UFUNCTION(BlueprintCallable, Category = "PB|Ball")
	void TeleportToAndStop(const FVector& Location, const FRotator& Rotation);

	UPBShotComponent* GetShotComponent() const { return ShotComponent; }

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_Attributes();

private:
	/** Push the effective (CVar-overridden) damping onto the body instance. */
	void ApplyPhysicsTuning();

	/** Server-only: throttle the net update rate by whether the ball is moving. */
	void UpdateNetUpdateRate();

	/** Last damping values written to the body, so Tick only re-pushes on change. */
	float LastLinearDampingApplied = -1.f;
	float LastAngularDampingApplied = -1.f;

	/** Last net update Hz written, so Tick only re-pushes the rate on change. */
	float LastNetHzApplied = -1.f;
};
