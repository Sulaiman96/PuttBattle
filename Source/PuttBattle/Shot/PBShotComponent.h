// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PBShotTypes.h"
#include "PBShotComponent.generated.h"

class APBBallPawn;
class APlayerController;
class UCurveFloat;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPBStrokeCountChanged, int32, NewStrokeCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPBAimUpdated, float, Power01, bool, bIndicatorHidden);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPBAimEnded);

/**
 * The drag-aim-release loop (plans/01 T1.2). Lives on APBBallPawn; the input
 * itself is captured by APBPlayerController, which feeds screen-space drag in
 * via StartAim/UpdateAim/ReleaseAim. Aim is camera-relative and strictly flat
 * (D3): we never read the deprojected world point, we rotate the screen drag by
 * the camera yaw and zero Z, so the shot plane is guaranteed horizontal.
 *
 * The single funnel ExecuteShot(FPBShotRequest) is, in Phase 3, the body of the
 * Server_ExecuteShot RPC — keep all authority-relevant logic inside it.
 *
 * Feel constants follow the same UPROPERTY + pb.Shot.* CVar pattern as the pawn
 * so UA-9 can tune the whole loop live in one PIE session.
 */
UCLASS(ClassGroup = (PB), meta = (BlueprintSpawnableComponent))
class PUTTBATTLE_API UPBShotComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPBShotComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// --- Input entry points (called by APBPlayerController) ----------------

	/** Press: begin aiming if the ball is at rest and not locked. */
	void StartAim(const FVector2D& ScreenPos);
	/** Drag: refresh the cached request + preview from the current cursor. */
	void UpdateAim(const FVector2D& ScreenPos);
	/** Release: fire if past the dead-zone, else cancel. */
	void ReleaseAim();
	/** Explicit cancel (RMB / Esc). */
	void CancelAim();

	/** True when a new shot may begin (Idle, at rest, not Lock'd). */
	UFUNCTION(BlueprintPure, Category = "PB|Shot")
	bool CanAim() const;

	UFUNCTION(BlueprintPure, Category = "PB|Shot")
	EPBShotState GetShotState() const { return ShotState; }

	UFUNCTION(BlueprintPure, Category = "PB|Shot")
	int32 GetStrokeCount() const { return StrokeCount; }

	/** Reset strokes + state (hole restart). */
	UFUNCTION(BlueprintCallable, Category = "PB|Shot")
	void ResetForNewHole();

	/** The one funnel every shot passes through (future server RPC body). */
	void ExecuteShot(const FPBShotRequest& Request);

	// --- HUD hooks ---------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "PB|Shot")
	FPBStrokeCountChanged OnStrokeCountChanged;

	UPROPERTY(BlueprintAssignable, Category = "PB|Shot")
	FPBAimUpdated OnAimUpdated;

	UPROPERTY(BlueprintAssignable, Category = "PB|Shot")
	FPBAimEnded OnAimEnded;

	// --- Feel constants (BP-tunable; mirrored by pb.Shot.* CVars) ----------

	/** Max drag length as a fraction of viewport height (D2). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Shot|Feel", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float MaxDragScreenFraction = 0.28f;

	/** Below this drag length (px) a release cancels instead of firing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Shot|Feel", meta = (ClampMin = "0.0"))
	float DeadZonePixels = 24.f;

	/** Linear[0,1] drag → eased power[0,1] (fine control at putting range). Identity if null. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Shot|Feel")
	TObjectPtr<UCurveFloat> PowerCurve;

	/** At-rest speed threshold (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Shot|Feel", meta = (ClampMin = "0.0"))
	float AtRestSpeedThreshold = 5.f;

	/** Sustained time below threshold before the ball counts as at rest (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Shot|Feel", meta = (ClampMin = "0.0"))
	float AtRestDuration = 0.5f;

	/** A still-rolling ball is force-stopped after this long (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Shot|Feel", meta = (ClampMin = "0.0"))
	float RollTimeout = 12.f;

protected:
	virtual void BeginPlay() override;

	/** BP preview hook (Niagara / spline mesh). Start→End along aim, length ∝ power. */
	UFUNCTION(BlueprintImplementableEvent, Category = "PB|Shot")
	void OnUpdateAimPreview(const FVector& Start, const FVector& End, float Power01);

	UFUNCTION(BlueprintImplementableEvent, Category = "PB|Shot")
	void OnHideAimPreview();

private:
	APBBallPawn* GetBall() const;
	APlayerController* GetOwningPC() const;

	/** Rebuild CachedRequest + preview endpoint from the current drag. */
	void RecomputeAim();
	void SetShotState(EPBShotState NewState);
	void UpdatePreview();

	UPROPERTY(Transient)
	EPBShotState ShotState = EPBShotState::Idle;

	/** Strokes this hole. Phase 1 lives here; moves to PlayerState in Phase 3/4. */
	UPROPERTY(Transient)
	int32 StrokeCount = 0;

	FVector2D PressScreenPos = FVector2D::ZeroVector;
	FVector2D CurrentScreenPos = FVector2D::ZeroVector;

	/** Last computed shot intent (flat dir + eased power). */
	FPBShotRequest CachedRequest;
	FVector PreviewEnd = FVector::ZeroVector;

	/** Seconds the ball has been continuously below the at-rest threshold. */
	float AtRestTimer = 0.f;
	/** Seconds since the current shot was fired (RollTimeout guard). */
	float RollTimer = 0.f;
};
