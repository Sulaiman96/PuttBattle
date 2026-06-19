// Copyright Epic Games, Inc. All Rights Reserved.

#include "PBShotComponent.h"

#include "Ball/PBBallPawn.h"
#include "Components/SphereComponent.h"
#include "Curves/CurveFloat.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Match/PBMatchGameState.h"
#include "Match/PBPlayerState.h"
#include "PBCollisionChannels.h"

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
	SetIsReplicatedByDefault(true); // needed so the owning client can route Server_RequestShot
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

		// Hold the Rolling lock until the ball is actually seen to move (covers the
		// RTT to the server, and a shot the server rejected / a zero-power no-op).
		// Until then we don't let at-rest detection flip us back to Idle, which would
		// otherwise allow a remote client to fire a second shot before the first lands.
		if (bWaitingForShotMotion)
		{
			if (Ball->GetSpeed() > RestSpeedEff)
			{
				bWaitingForShotMotion = false; // motion confirmed; resume normal detection
			}
			else if (ShotConfirmTimeout > 0.f && RollTimer >= ShotConfirmTimeout)
			{
				SetShotState(EPBShotState::Idle); // shot never moved the ball — release the lock
			}
			return;
		}

		const bool bAtRest = AtRestTimer >= AtRestDuration;
		const bool bTimedOut = RollTimeoutEff > 0.f && RollTimer >= RollTimeoutEff;
		// Force-stop is a physics mutation → authority only; clients get it via rep.
		if (bTimedOut && Ball->HasAuthority())
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
	// In a match, only aim during shot-allowed phases; offline (no match GameState)
	// stays permissive so the graybox feel-test map still works.
	if (const UWorld* World = GetWorld())
	{
		if (const APBMatchGameState* GS = World->GetGameState<APBMatchGameState>())
		{
			if (!GS->ArePlayerShotsAllowed())
			{
				return false;
			}
		}
	}
	if (const APBPlayerState* PS = Ball->GetPlayerState<APBPlayerState>())
	{
		if (PS->IsFinished())
		{
			return false; // finished players spectate (D13) — no more shots this hole
		}
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

	// Owning-client cosmetic feedback the instant the player releases, before the
	// server round-trip (T3.1 local-feel). The authoritative impulse is server-side.
	OnLocalShotFired(CachedRequest.Power01);

	// Route to the server. The listen-server host is authority and runs the server
	// path directly; a remote client sends the validated RPC.
	if (APBBallPawn* Ball = GetBall(); Ball && Ball->HasAuthority())
	{
		TryExecuteServerShot(CachedRequest);
	}
	else
	{
		Server_RequestShot(CachedRequest);
	}

	// Enter the local rolling lock; held until the ball is observed to move so a
	// queued shot can't be double-fired (see TickComponent).
	RollTimer = 0.f;
	AtRestTimer = 0.f;
	bWaitingForShotMotion = true;
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

	// Preview endpoint: trace from the ball along the aim dir, clipped at the first
	// wall/floor it would hit. Trace by the PB object channels (not ECC_Visibility,
	// which the §4 collision contract makes no promise about) — mirrors the sampler.
	const FVector Start = Ball->GetActorLocation();
	const float PreviewLen = FMath::Max(PreviewMinLength, Power * PreviewLengthAtFullPower); // length encodes power
	FVector End = Start + AimDir * PreviewLen;

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PBAimPreview), false, Ball);
	FCollisionObjectQueryParams ObjParams;
	ObjParams.AddObjectTypesToQuery(PBCollision::Wall);
	ObjParams.AddObjectTypesToQuery(PBCollision::Floor);
	if (GetWorld() && GetWorld()->LineTraceSingleByObjectType(Hit, Start, End, ObjParams, Params))
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

	// Server-side funnel (called from TryExecuteServerShot). Defends its own input
	// (CONVENTIONS §2): clamp the client-supplied power to [0,1] here, not just
	// upstream in RecomputeAim, so a forged RPC payload can't over-power a shot.
	const float SafePower = FMath::Clamp(Request.Power01, 0.f, 1.f);

	// Flat impulse only (D3). Mass matters (bVelChange=false) so a Ballooned,
	// heavier ball travels less from the same shot.
	const FVector Dir3D(Request.Dir.X, Request.Dir.Y, 0.f);
	const float Magnitude = SafePower
		* Ball->GetEffectiveMaxImpulse()
		* FMath::Max(Ball->Attributes.PowerMultiplier, 0.f);

	Ball->CollisionSphere->AddImpulse(Dir3D.GetSafeNormal() * Magnitude, NAME_None, /*bVelChange=*/false);

	// Strokes are authoritative state on the PlayerState — the client never runs
	// this funnel, so the count only advances here on the server.
	if (APBPlayerState* PS = Ball->GetPlayerState<APBPlayerState>())
	{
		PS->AddStroke();
	}
}

void UPBShotComponent::ResetForNewHole()
{
	// Local state machine only — strokes live on the PlayerState (reset there by
	// the GameMode's RestartHole).
	AtRestTimer = 0.f;
	RollTimer = 0.f;
	bWaitingForShotMotion = false;
	SetShotState(EPBShotState::Idle);
}

bool UPBShotComponent::IsShotRequestStructurallyValid(const FPBShotRequest& Request)
{
	const bool bFiniteDir = FMath::IsFinite(Request.Dir.X) && FMath::IsFinite(Request.Dir.Y);
	const bool bFinitePower = FMath::IsFinite(Request.Power01);
	return bFiniteDir && bFinitePower
		&& Request.Power01 >= -KINDA_SMALL_NUMBER
		&& Request.Power01 <= 1.f + KINDA_SMALL_NUMBER;
}

bool UPBShotComponent::Server_RequestShot_Validate(const FPBShotRequest& Request)
{
	// Cheat guard only (a failed validate disconnects the sender): reject a
	// structurally-impossible payload. State gates (phase, at-rest, lock, finished)
	// are re-checked in _Implementation and silently ignored so an honest, slightly
	// mistimed shot under latency never kicks the player.
	return IsShotRequestStructurallyValid(Request);
}

void UPBShotComponent::Server_RequestShot_Implementation(const FPBShotRequest& Request)
{
	TryExecuteServerShot(Request);
}

bool UPBShotComponent::ServerShotStateAllows() const
{
	const APBBallPawn* Ball = GetBall();
	if (!Ball)
	{
		return false;
	}
	// The component ticks on the server for every ball, so AtRestTimer is the
	// authority's own at-rest measurement. Reject only a genuinely mid-shot ball
	// (Rolling) or one not yet settled — NOT Aiming: on the listen-server host and
	// in standalone, the authority releases its own shot straight from Aiming and
	// runs this gate directly (ReleaseAim → TryExecuteServerShot) *before* flipping
	// to Rolling. (For a remote client the server's copy of this component is Idle,
	// so rejecting-Rolling is equivalent to the old rejecting-non-Idle there.)
	if (ShotState == EPBShotState::Rolling || AtRestTimer < AtRestDuration)
	{
		return false;
	}
	if (Ball->Attributes.bShotLocked)
	{
		return false; // Lock (CONVENTIONS §3)
	}
	if (const UWorld* World = GetWorld())
	{
		if (const APBMatchGameState* GS = World->GetGameState<APBMatchGameState>())
		{
			if (!GS->ArePlayerShotsAllowed())
			{
				return false; // wrong phase (Countdown / HoleResults / …)
			}
		}
	}
	if (const APBPlayerState* PS = Ball->GetPlayerState<APBPlayerState>())
	{
		if (PS->IsFinished())
		{
			return false; // already sunk this hole
		}
	}
	return true;
}

void UPBShotComponent::TryExecuteServerShot(const FPBShotRequest& Request)
{
	if (!ServerShotStateAllows())
	{
		return; // silently rejected — honest clients see nothing, cheats gain nothing
	}

	ExecuteShot(Request);

	// Server enters the same rolling lock so a re-entrant RPC is rejected until the
	// ball comes to rest again (ServerShotStateAllows gates on ShotState + at-rest).
	RollTimer = 0.f;
	AtRestTimer = 0.f;
	bWaitingForShotMotion = true;
	SetShotState(EPBShotState::Rolling);
}

void UPBShotComponent::SetShotState(EPBShotState NewState)
{
	ShotState = NewState;
}
