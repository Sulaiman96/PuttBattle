// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PBShotTypes.h"
#include "PBShotComponent.generated.h"

class APBBallPawn;
class APlayerController;
class UCurveFloat;

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

	/** Reset the local shot state machine (hole restart). Strokes live on the
	 *  PlayerState now and are reset there by the GameMode. */
	UFUNCTION(BlueprintCallable, Category = "PB|Shot")
	void ResetForNewHole();

	/**
	 * Client→server shot request (CONVENTIONS §2). The release of a drag routes
	 * here on a remote client; the listen-server host runs the same server path
	 * directly. WithValidation rejects structurally-invalid payloads (cheat
	 * guard); state gates (phase, at-rest, lock, finished) are re-checked in the
	 * implementation and silently ignored so honest latency never kicks a client.
	 */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestShot(const FPBShotRequest& Request);

	/** The one funnel every accepted shot passes through (server-side): applies the
	 *  flat impulse and adds a stroke on the PlayerState. */
	void ExecuteShot(const FPBShotRequest& Request);

	/**
	 * Pure cheat-guard predicate for an incoming shot payload (finite dir/power,
	 * power within [0,1] + FP slack). Shared by Server_RequestShot_Validate and the
	 * automation test so the exploit rejection is regression-checked (T3.1).
	 */
	static bool IsShotRequestStructurallyValid(const FPBShotRequest& Request);

	// --- HUD hooks ---------------------------------------------------------

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

	/** After firing, how long to wait for the ball to start moving before giving up
	 *  the Rolling lock (covers RTT to the server + a rejected/zero-power shot, so a
	 *  remote client can't double-fire while the first shot is still in transit). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Shot|Feel", meta = (ClampMin = "0.0"))
	float ShotConfirmTimeout = 1.5f;

	/** Aim-preview length (cm) at full power — the visual the player calibrates against. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Shot|Feel", meta = (ClampMin = "0.0"))
	float PreviewLengthAtFullPower = 600.f;

	/** Shortest visible aim-preview length (cm) so a tiny tap still shows a line. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Shot|Feel", meta = (ClampMin = "0.0"))
	float PreviewMinLength = 50.f;

protected:
	/** BP preview hook (Niagara / spline mesh). Start→End along aim, length ∝ power. */
	UFUNCTION(BlueprintImplementableEvent, Category = "PB|Shot")
	void OnUpdateAimPreview(const FVector& Start, const FVector& End, float Power01);

	UFUNCTION(BlueprintImplementableEvent, Category = "PB|Shot")
	void OnHideAimPreview();

	/** Owning-client cosmetic feedback fired the instant the player releases (swing
	 *  SFX/VFX), independent of the server round-trip (T3.1 local-feel). */
	UFUNCTION(BlueprintImplementableEvent, Category = "PB|Shot")
	void OnLocalShotFired(float Power01);

private:
	APBBallPawn* GetBall() const;
	APlayerController* GetOwningPC() const;

	/** Server-side: state gates a shot must pass (phase, at-rest, lock, finished). */
	bool ServerShotStateAllows() const;

	/** Server-side: validate state then run ExecuteShot + enter the rolling lock. */
	void TryExecuteServerShot(const FPBShotRequest& Request);

	/** Rebuild CachedRequest + preview endpoint from the current drag. */
	void RecomputeAim();
	void SetShotState(EPBShotState NewState);
	void UpdatePreview();

	UPROPERTY(Transient)
	EPBShotState ShotState = EPBShotState::Idle;

	FVector2D PressScreenPos = FVector2D::ZeroVector;
	FVector2D CurrentScreenPos = FVector2D::ZeroVector;

	/** Last computed shot intent (flat dir + eased power). */
	FPBShotRequest CachedRequest;
	FVector PreviewEnd = FVector::ZeroVector;

	/** Seconds the ball has been continuously below the at-rest threshold. */
	float AtRestTimer = 0.f;
	/** Seconds since the current shot was fired (RollTimeout guard). */
	float RollTimer = 0.f;

	/** True between firing and observing the ball actually move — holds the Rolling
	 *  lock so a queued shot can't be double-fired before it replicates back. */
	bool bWaitingForShotMotion = false;
};
