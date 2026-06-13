// Copyright Epic Games, Inc. All Rights Reserved.

#include "PBShotComponent.h"

#include "Ball/PBBallPawn.h"
#include "Components/SphereComponent.h"
#include "Curves/CurveFloat.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"

namespace PBShotCVars
{
	static TAutoConsoleVariable<float> CVarMaxDragFraction(
		TEXT("pb.Shot.MaxDragFraction"), -1.f,
		TEXT("Override max drag as a fraction of viewport height (>=0). -1 = authored."),
		ECVF_Default);

	static TAutoConsoleVariable<float> CVarDeadZone(
		TEXT("pb.Shot.DeadZone"), -1.f,
		TEXT("Override the release-cancel dead-zone in pixels (>=0). -1 = authored."),
		ECVF_Default);

	static TAutoConsoleVariable<float> CVarAtRestSpeed(
		TEXT("pb.Shot.AtRestSpeed"), -1.f,
		TEXT("Override the at-rest speed threshold in cm/s (>=0). -1 = authored."),
		ECVF_Default);

	static TAutoConsoleVariable<float> CVarRollTimeout(
		TEXT("pb.Shot.RollTimeout"), -1.f,
		TEXT("Override the roll force-stop timeout in seconds (>=0). -1 = authored."),
		ECVF_Default);

	static TAutoConsoleVariable<int32> CVarDrawPreview(
		TEXT("pb.Shot.DrawDebugPreview"), 1,
		TEXT("Draw a debug aim line while aiming (1) until the BP Niagara preview is wired (0)."),
		ECVF_Default);
}

UPBShotComponent::UPBShotComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(false); // shot input is local; Phase 3 adds the server RPC path
}

void UPBShotComponent::BeginPlay()
{
	Super::BeginPlay();
}

APBBallPawn* UPBShotComponent::GetBall() const
{
	return Cast<APBBallPawn>(GetOwner());
}

APlayerController* UPBShotComponent::GetOwningPC() const
{
	const APBBallPawn* Ball = GetBall();
	return Ball ? Cast<APlayerController>(Ball->GetController()) : nullptr;
}

void UPBShotComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	APBBallPawn* Ball = GetBall();
	if (!Ball)
	{
		return;
	}

	const float RestSpeedEff = PBShotCVars::CVarAtRestSpeed.GetValueOnGameThread() >= 0.f
		? PBShotCVars::CVarAtRestSpeed.GetValueOnGameThread() : AtRestSpeedThreshold;
	const float RollTimeoutEff = PBShotCVars::CVarRollTimeout.GetValueOnGameThread() >= 0.f
		? PBShotCVars::CVarRollTimeout.GetValueOnGameThread() : RollTimeout;

	// At-rest detection: accumulate time under the speed threshold.
	if (Ball->GetSpeed() <= RestSpeedEff)
	{
		AtRestTimer += DeltaTime;
	}
	else
	{
		AtRestTimer = 0.f;
	}

	if (ShotState == EPBShotState::Rolling)
	{
		RollTimer += DeltaTime;
		const bool bAtRest = AtRestTimer >= AtRestDuration;
		const bool bTimedOut = RollTimeoutEff > 0.f && RollTimer >= RollTimeoutEff;
		if (bTimedOut)
		{
			Ball->StopMotion();
		}
		if (bAtRest || bTimedOut)
		{
			SetShotState(EPBShotState::Idle);
		}
	}
	else if (ShotState == EPBShotState::Aiming)
	{
		UpdatePreview();
	}
}

bool UPBShotComponent::CanAim() const
{
	const APBBallPawn* Ball = GetBall();
	if (!Ball || ShotState != EPBShotState::Idle)
	{
		return false;
	}
	if (Ball->Attributes.bShotLocked)
	{
		return false; // Lock powerup refuses the shot (CONVENTIONS §3)
	}
	return AtRestTimer >= AtRestDuration;
}

void UPBShotComponent::StartAim(const FVector2D& ScreenPos)
{
	if (!CanAim())
	{
		return;
	}
	PressScreenPos = ScreenPos;
	CurrentScreenPos = ScreenPos;
	CachedRequest = FPBShotRequest();
	SetShotState(EPBShotState::Aiming);
	RecomputeAim();
}

void UPBShotComponent::UpdateAim(const FVector2D& ScreenPos)
{
	if (ShotState != EPBShotState::Aiming)
	{
		return;
	}
	CurrentScreenPos = ScreenPos;
	RecomputeAim();
}

void UPBShotComponent::ReleaseAim()
{
	if (ShotState != EPBShotState::Aiming)
	{
		return;
	}

	const float DeadZoneEff = PBShotCVars::CVarDeadZone.GetValueOnGameThread() >= 0.f
		? PBShotCVars::CVarDeadZone.GetValueOnGameThread() : DeadZonePixels;
	const float DragLen = (CurrentScreenPos - PressScreenPos).Size();

	if (DragLen < DeadZoneEff || CachedRequest.Power01 <= KINDA_SMALL_NUMBER)
	{
		CancelAim();
		return;
	}

	ExecuteShot(CachedRequest);
	RollTimer = 0.f;
	AtRestTimer = 0.f;
	SetShotState(EPBShotState::Rolling);
	OnHideAimPreview();
	OnAimEnded.Broadcast();
}

void UPBShotComponent::CancelAim()
{
	if (ShotState != EPBShotState::Aiming)
	{
		return;
	}
	SetShotState(EPBShotState::Idle);
	OnHideAimPreview();
	OnAimEnded.Broadcast();
}

void UPBShotComponent::RecomputeAim()
{
	APBBallPawn* Ball = GetBall();
	APlayerController* PC = GetOwningPC();
	if (!Ball || !PC || !PC->PlayerCameraManager)
	{
		return;
	}

	// Camera-relative, flat (D3): rotate the screen drag by camera yaw, zero Z.
	const FRotator CamRot = PC->PlayerCameraManager->GetCameraRotation();
	FVector Fwd = FRotationMatrix(CamRot).GetUnitAxis(EAxis::X);
	FVector Right = FRotationMatrix(CamRot).GetUnitAxis(EAxis::Y);
	Fwd.Z = 0.f; Right.Z = 0.f;
	Fwd.Normalize(); Right.Normalize();

	const FVector2D Drag = CurrentScreenPos - PressScreenPos; // screen px, +Y is down
	FVector DragWorld = (Right * Drag.X) + (Fwd * -Drag.Y);

	// Ball travels opposite the drag (slingshot) unless Reverse is active.
	FVector AimDir = Ball->Attributes.bAimInverted ? DragWorld : -DragWorld;
	AimDir.Z = 0.f;
	AimDir = AimDir.GetSafeNormal();

	// Power: clamped drag length through the ease curve.
	FVector2D ViewportSize(1920.f, 1080.f);
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}
	const float MaxFracEff = PBShotCVars::CVarMaxDragFraction.GetValueOnGameThread() >= 0.f
		? PBShotCVars::CVarMaxDragFraction.GetValueOnGameThread() : MaxDragScreenFraction;
	const float MaxDragPx = MaxFracEff * ViewportSize.Y;
	const float RawPower = MaxDragPx > 0.f ? FMath::Clamp(Drag.Size() / MaxDragPx, 0.f, 1.f) : 0.f;
	const float Power = PowerCurve ? FMath::Clamp(PowerCurve->GetFloatValue(RawPower), 0.f, 1.f) : RawPower;

	CachedRequest.Dir = FVector2D(AimDir.X, AimDir.Y);
	CachedRequest.Power01 = Power;

	// Preview endpoint: trace from the ball along the aim dir to the first wall.
	const FVector Start = Ball->GetActorLocation();
	const float PreviewLen = FMath::Max(50.f, Power * 600.f); // length encodes power
	FVector End = Start + AimDir * PreviewLen;

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PBAimPreview), false, Ball);
	if (GetWorld() && GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		End = Hit.ImpactPoint;
	}
	PreviewEnd = End;
}

void UPBShotComponent::UpdatePreview()
{
	APBBallPawn* Ball = GetBall();
	if (!Ball)
	{
		return;
	}

	const bool bHidden = Ball->Attributes.bAimIndicatorHidden; // Jumble
	OnAimUpdated.Broadcast(CachedRequest.Power01, bHidden);

	if (bHidden)
	{
		OnHideAimPreview();
		return;
	}

	OnUpdateAimPreview(Ball->GetActorLocation(), PreviewEnd, CachedRequest.Power01);

	if (PBShotCVars::CVarDrawPreview.GetValueOnGameThread() != 0)
	{
		DrawDebugLine(GetWorld(), Ball->GetActorLocation(), PreviewEnd,
			FColor::MakeRedToGreenColorFromScalar(1.f - CachedRequest.Power01),
			false, -1.f, 0, 1.5f);
	}
}

void UPBShotComponent::ExecuteShot(const FPBShotRequest& Request)
{
	APBBallPawn* Ball = GetBall();
	if (!Ball || !Ball->CollisionSphere)
	{
		return;
	}
	if (Ball->Attributes.bShotLocked)
	{
		return;
	}

	// Flat impulse only (D3). Mass matters (bVelChange=false) so a Ballooned,
	// heavier ball travels less from the same shot.
	const FVector Dir3D(Request.Dir.X, Request.Dir.Y, 0.f);
	const float Magnitude = Request.Power01
		* Ball->GetEffectiveMaxImpulse()
		* FMath::Max(Ball->Attributes.PowerMultiplier, 0.f);

	Ball->CollisionSphere->AddImpulse(Dir3D.GetSafeNormal() * Magnitude, NAME_None, /*bVelChange=*/false);

	++StrokeCount;
	OnStrokeCountChanged.Broadcast(StrokeCount);
}

void UPBShotComponent::ResetForNewHole()
{
	StrokeCount = 0;
	AtRestTimer = 0.f;
	RollTimer = 0.f;
	SetShotState(EPBShotState::Idle);
	OnStrokeCountChanged.Broadcast(StrokeCount);
}

void UPBShotComponent::SetShotState(EPBShotState NewState)
{
	ShotState = NewState;
}
