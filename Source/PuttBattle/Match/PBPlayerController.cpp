// Copyright Epic Games, Inc. All Rights Reserved.

#include "PBPlayerController.h"

#include "Ball/PBBallPawn.h"
#include "Camera/CameraComponent.h"
#include "Camera/PBCameraRig.h"
#include "Camera/PBCameraSubsystem.h"
#include "Course/PBCupActor.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Match/PBLobbyGameMode.h"
#include "Match/PBPlayerState.h"
#include "Shot/PBShotComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/SpringArmComponent.h"

namespace PBCameraInputCVars
{
	// Feel knobs for the click-to-aim / drag-to-orbit split (CONVENTIONS §1). Live in
	// PIE: `pb.Camera.AimClickScale 2.0`, `pb.Camera.OrbitSpeed 0.5`.
	static TAutoConsoleVariable<float> CVarAimClickScale(
		TEXT("pb.Camera.AimClickScale"), 1.6f,
		TEXT("Clickable radius around the ball as a multiple of its on-screen size: a press inside aims, outside orbits the camera."),
		ECVF_Default);

	static TAutoConsoleVariable<float> CVarOrbitSpeed(
		TEXT("pb.Camera.OrbitSpeed"), -1.f,
		TEXT("Override RMB orbit yaw (degrees per pixel of horizontal drag). -1 = use the controller's authored OrbitYawSpeed."),
		ECVF_Default);

	static TAutoConsoleVariable<float> CVarFaceHoleSpeed(
		TEXT("pb.Camera.FaceHoleSpeed"), 3.f,
		TEXT("How fast cam 2 eases its yaw to keep the hole framed (higher = snappier)."),
		ECVF_Default);
}

namespace
{
	/** True when the screen-space cursor falls inside the ball's (scaled) on-screen circle. */
	bool IsCursorOverBall(APlayerController* PC, APBBallPawn* Ball, const FVector2D& Cursor, float RadiusScale)
	{
		if (!PC || !Ball || !Ball->CollisionSphere)
		{
			return false;
		}
		const FVector BallLoc = Ball->GetActorLocation();
		FVector2D BallScreen;
		if (!PC->ProjectWorldLocationToScreen(BallLoc, BallScreen))
		{
			return false;
		}
		// Project a point one (scaled) ball-radius to camera-right so the on-screen
		// click radius shrinks naturally as the ball gets further from the camera.
		FVector CamRight = FVector::RightVector;
		if (PC->PlayerCameraManager)
		{
			CamRight = FRotationMatrix(PC->PlayerCameraManager->GetCameraRotation()).GetUnitAxis(EAxis::Y);
		}
		const float WorldRadius = Ball->CollisionSphere->GetScaledSphereRadius() * FMath::Max(RadiusScale, 1.f);
		FVector2D EdgeScreen;
		if (!PC->ProjectWorldLocationToScreen(BallLoc + CamRight * WorldRadius, EdgeScreen))
		{
			return false;
		}
		const float ScreenRadius = FMath::Max(FVector2D::Distance(BallScreen, EdgeScreen), 10.f); // floor: a tiny far ball stays clickable
		return FVector2D::Distance(Cursor, BallScreen) <= ScreenRadius;
	}

	/** Orbit the ball's spring-arm camera horizontally. The boom uses absolute rotation
	 *  (D-7), so its relative rotation is its world rotation — bumping yaw turns the view
	 *  around the ball while the 1/2/3 presets keep owning pitch + FOV. */
	void OrbitBallCamera(APBBallPawn* Ball, float YawDelta)
	{
		if (!Ball)
		{
			return;
		}
		if (USpringArmComponent* Boom = Ball->FindComponentByClass<USpringArmComponent>())
		{
			FRotator Rot = Boom->GetRelativeRotation();
			Rot.Yaw += YawDelta;
			Boom->SetRelativeRotation(Rot);
		}
	}
}

APBPlayerController::APBPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	// Default framings (retune on BP_PlayerController or live). cam 1 = a low "play"
	// angle you can orbit with RMB; cam 2 = pulled back and auto-facing the hole so
	// the ball and cup share the frame. cam 3 (the map overview) is a level rig, not
	// a follow preset — selecting the slot past the last preset switches to it.
	FPBFollowCamPreset Play;
	Play.Label = TEXT("Play");
	Play.Pitch = -30.f;
	Play.ArmLength = 650.f;
	Play.FieldOfView = 75.f;
	Play.bAllowOrbit = true;
	Play.bFaceHole = false;

	FPBFollowCamPreset Shot;
	Shot.Label = TEXT("Shot");
	Shot.Pitch = -45.f;
	Shot.ArmLength = 1150.f;
	Shot.FieldOfView = 80.f;
	Shot.bAllowOrbit = false;
	Shot.bFaceHole = true;

	FollowCamPresets = { Play, Shot };
}

void APBPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (PuttMappingContext)
		{
			Subsystem->AddMappingContext(PuttMappingContext, MappingContextPriority);
		}
	}

	// Aim is a screen-space drag — keep the cursor available over the game viewport.
	SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false));

	// Seed the default framing. If the pawn isn't possessed yet this no-ops and the
	// boom keeps its BP default pose (which matches cam 1); pressing 1 re-applies.
	ApplyCameraMode();
}

void APBPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (DragShotAction)
		{
			EIC->BindAction(DragShotAction, ETriggerEvent::Started, this, &APBPlayerController::OnDragStarted);
			EIC->BindAction(DragShotAction, ETriggerEvent::Completed, this, &APBPlayerController::OnDragReleased);
			EIC->BindAction(DragShotAction, ETriggerEvent::Canceled, this, &APBPlayerController::OnDragReleased);
		}
		if (CycleCameraAction)
		{
			EIC->BindAction(CycleCameraAction, ETriggerEvent::Started, this, &APBPlayerController::OnCycleCamera);
		}
		if (CancelAction)
		{
			EIC->BindAction(CancelAction, ETriggerEvent::Started, this, &APBPlayerController::OnCancel);
		}
		// Direct framing selection on keys 1/2/3.
		if (Camera1Action)
		{
			EIC->BindAction(Camera1Action, ETriggerEvent::Started, this, &APBPlayerController::OnSelectCamera1);
		}
		if (Camera2Action)
		{
			EIC->BindAction(Camera2Action, ETriggerEvent::Started, this, &APBPlayerController::OnSelectCamera2);
		}
		if (Camera3Action)
		{
			EIC->BindAction(Camera3Action, ETriggerEvent::Started, this, &APBPlayerController::OnSelectCamera3);
		}
		// RMB hold = orbit the play camera (drag left/right to turn).
		if (OrbitCameraAction)
		{
			EIC->BindAction(OrbitCameraAction, ETriggerEvent::Started, this, &APBPlayerController::OnOrbitStarted);
			EIC->BindAction(OrbitCameraAction, ETriggerEvent::Completed, this, &APBPlayerController::OnOrbitReleased);
			EIC->BindAction(OrbitCameraAction, ETriggerEvent::Canceled, this, &APBPlayerController::OnOrbitReleased);
		}
	}
}

void APBPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// LMB drag streams the aim to the shot component. Camera orbit moved to RMB
	// (handled in UpdateActiveCamera), so a click off the ball no longer turns the view.
	if (bDragging)
	{
		UPBShotComponent* Shot = GetShotComponent();
		if (Shot && Shot->GetShotState() == EPBShotState::Aiming)
		{
			// Aim drag: power + direction track the mouse every frame.
			FVector2D Cursor;
			if (GetCursorScreenPos(Cursor))
			{
				Shot->UpdateAim(Cursor);
			}
		}
	}

	// Face-the-hole easing (cam 2) and RMB orbit (cam 1) run every frame.
	UpdateActiveCamera(DeltaTime);
}

void APBPlayerController::OnDragStarted(const FInputActionValue& Value)
{
	FVector2D Cursor;
	if (!GetCursorScreenPos(Cursor))
	{
		return;
	}
	bDragging = true; // a drag is underway; PlayerTick streams aim updates while it's set

	// Press on (a slightly-enlarged) ball begins a shot; pressing off the ball does
	// nothing now (camera turning moved to RMB — see OnOrbitStarted / UpdateActiveCamera).
	APBBallPawn* Ball = Cast<APBBallPawn>(GetPawn());
	const float ClickScale = PBCameraInputCVars::CVarAimClickScale.GetValueOnGameThread();
	if (Ball && IsCursorOverBall(this, Ball, Cursor, ClickScale))
	{
		if (UPBShotComponent* Shot = Ball->GetShotComponent())
		{
			Shot->StartAim(Cursor);
		}
	}
}

void APBPlayerController::OnDragReleased(const FInputActionValue& Value)
{
	if (!bDragging)
	{
		return;
	}
	bDragging = false;
	if (UPBShotComponent* Shot = GetShotComponent())
	{
		Shot->ReleaseAim();
	}
}

void APBPlayerController::OnCycleCamera(const FInputActionValue& Value)
{
	CycleCameraMode();
}

void APBPlayerController::OnSelectCamera1(const FInputActionValue& Value)
{
	SelectCameraMode(0);
}

void APBPlayerController::OnSelectCamera2(const FInputActionValue& Value)
{
	SelectCameraMode(1);
}

void APBPlayerController::OnSelectCamera3(const FInputActionValue& Value)
{
	// cam 3 is always the overview rig — the slot just past the follow presets.
	SelectCameraMode(FollowCamPresets.Num());
}

void APBPlayerController::OnOrbitStarted(const FInputActionValue& Value)
{
	bOrbiting = true;
}

void APBPlayerController::OnOrbitReleased(const FInputActionValue& Value)
{
	bOrbiting = false;
}

void APBPlayerController::SelectCameraMode(int32 Index)
{
	// Valid modes: 0..Num()-1 = follow framings, Num() = the overview rig slot.
	CameraMode = FMath::Clamp(Index, 0, FollowCamPresets.Num());
	ApplyCameraMode();
}

void APBPlayerController::CycleCameraMode()
{
	const int32 Modes = FollowCamPresets.Num() + (GetOrbitRig() ? 1 : 0);
	if (Modes <= 0)
	{
		return;
	}
	CameraMode = (CameraMode + 1) % Modes;
	ApplyCameraMode();
}

void APBPlayerController::ApplyCameraMode()
{
	APBBallPawn* Ball = Cast<APBBallPawn>(GetPawn());
	const int32 NumFollow = FollowCamPresets.Num();

	// Overview slot (cam 3): blend to the auto-orbit world rig if one is placed.
	if (NumFollow == 0 || CameraMode >= NumFollow)
	{
		if (APBCameraRig* Rig = GetOrbitRig())
		{
			SetViewTargetWithBlend(Rig, 0.5f);
			return;
		}
		// No overview rig in the level — clamp back to the last follow framing.
		CameraMode = FMath::Max(NumFollow - 1, 0);
	}

	if (NumFollow == 0 || !Ball)
	{
		return; // no presets, or pawn not possessed yet — boom keeps its BP pose
	}

	const FPBFollowCamPreset& P = FollowCamPresets[FMath::Clamp(CameraMode, 0, NumFollow - 1)];
	if (USpringArmComponent* Boom = Ball->FindComponentByClass<USpringArmComponent>())
	{
		Boom->TargetArmLength = P.ArmLength;
		FRotator R = Boom->GetRelativeRotation();
		R.Pitch = P.Pitch; // yaw is left alone (orbit / face-hole own it); roll flat
		R.Roll = 0.f;
		Boom->SetRelativeRotation(R);
	}
	if (UCameraComponent* Cam = Ball->FindComponentByClass<UCameraComponent>())
	{
		Cam->SetFieldOfView(P.FieldOfView);
	}
	// Return the view to the ball in case we were on the overview rig.
	SetViewTargetWithBlend(Ball, 0.3f);
}

void APBPlayerController::UpdateActiveCamera(float DeltaTime)
{
	APBBallPawn* Ball = Cast<APBBallPawn>(GetPawn());
	const int32 NumFollow = FollowCamPresets.Num();
	if (!Ball || CameraMode >= NumFollow)
	{
		return; // overview rig animates itself — nothing for the controller to do
	}

	USpringArmComponent* Boom = Ball->FindComponentByClass<USpringArmComponent>();
	if (!Boom)
	{
		return;
	}
	const FPBFollowCamPreset& P = FollowCamPresets[CameraMode];

	if (P.bFaceHole)
	{
		// Ease the boom forward toward ball -> cup so the camera (which sits behind
		// the ball) looks across it at the hole — both stay framed. Shortest-path
		// turn so it never spins the long way around the ±180° seam.
		if (APBCupActor* Cup = GetCup())
		{
			const FVector ToCup = Cup->GetActorLocation() - Ball->GetActorLocation();
			if (!ToCup.IsNearlyZero())
			{
				FRotator R = Boom->GetRelativeRotation();
				const float Delta = FMath::FindDeltaAngleDegrees(R.Yaw, ToCup.Rotation().Yaw);
				const float Speed = PBCameraInputCVars::CVarFaceHoleSpeed.GetValueOnGameThread();
				R.Yaw += Delta * FMath::Clamp(FMath::Max(Speed, 0.f) * DeltaTime, 0.f, 1.f);
				Boom->SetRelativeRotation(R);
			}
		}
	}
	else if (bOrbiting && P.bAllowOrbit)
	{
		// RMB drag: yaw the boom by the horizontal mouse delta.
		float DX = 0.f, DY = 0.f;
		GetInputMouseDelta(DX, DY);
		if (!FMath::IsNearlyZero(DX))
		{
			const float CVar = PBCameraInputCVars::CVarOrbitSpeed.GetValueOnGameThread();
			const float Speed = CVar >= 0.f ? CVar : OrbitYawSpeed;
			OrbitBallCamera(Ball, DX * Speed);
		}
	}
}

APBCupActor* APBPlayerController::GetCup()
{
	if (CachedCup.IsValid())
	{
		return CachedCup.Get();
	}
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<APBCupActor> It(World); It; ++It)
		{
			CachedCup = *It;
			return *It; // Phase 1: one hole, one cup
		}
	}
	return nullptr;
}

APBCameraRig* APBPlayerController::GetOrbitRig()
{
	if (CachedOrbitRig.IsValid())
	{
		return CachedOrbitRig.Get();
	}
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<APBCameraRig> It(World); It; ++It)
		{
			if (It->bAutoOrbit)
			{
				CachedOrbitRig = *It;
				return *It;
			}
		}
	}
	return nullptr;
}

void APBPlayerController::OnCancel(const FInputActionValue& Value)
{
	bDragging = false;
	if (UPBShotComponent* Shot = GetShotComponent())
	{
		Shot->CancelAim();
	}
}

void APBPlayerController::EnterSpectate()
{
	// Server-authoritative entry (called from the match GameMode on sink). Tell the
	// owning client to hold the placeholder camera; on the listen-server host this
	// PC is local authority, so the Client RPC simply runs locally.
	if (HasAuthority())
	{
		Client_EnterSpectate();
	}
}

void APBPlayerController::ExitSpectate()
{
	if (HasAuthority())
	{
		Client_ExitSpectate();
	}
}

void APBPlayerController::Client_EnterSpectate_Implementation()
{
	bSpectating = true;
	OnEnterSpectate(); // BP placeholder: swap to a fixed cam (real spectate is Phase 8)
}

void APBPlayerController::Client_ExitSpectate_Implementation()
{
	if (!bSpectating)
	{
		return; // not spectating — nothing to restore
	}
	bSpectating = false;
	OnExitSpectate(); // BP placeholder: restore the gameplay camera
}

// _Validate: these carry player-supplied input, so §2 wants WithValidation. The real
// authority gates (host-only, range clamps, all-ready) live server-side in the lobby
// GameMode, so validation only needs to confirm the payload isn't garbage.
bool APBPlayerController::Server_SetReady_Validate(bool bReady) { return true; }

bool APBPlayerController::Server_SetMatchSettings_Validate(FPBMatchSettings NewSettings)
{
	return FMath::IsFinite(NewSettings.MapTimeSeconds) && FMath::IsFinite(NewSettings.FirstFinishClampSeconds);
}

bool APBPlayerController::Server_RequestStartMatch_Validate() { return true; }

void APBPlayerController::Server_SetReady_Implementation(bool bReady)
{
	if (APBPlayerState* PS = GetPlayerState<APBPlayerState>())
	{
		PS->SetReady(bReady);
	}
}

void APBPlayerController::Server_SetMatchSettings_Implementation(FPBMatchSettings NewSettings)
{
	if (APBLobbyGameMode* Lobby = GetWorld() ? GetWorld()->GetAuthGameMode<APBLobbyGameMode>() : nullptr)
	{
		Lobby->HostSetMatchSettings(this, NewSettings); // GameMode re-checks host + clamps
	}
}

void APBPlayerController::Server_RequestStartMatch_Implementation()
{
	if (APBLobbyGameMode* Lobby = GetWorld() ? GetWorld()->GetAuthGameMode<APBLobbyGameMode>() : nullptr)
	{
		Lobby->RequestStartMatch(this); // GameMode re-checks host + all-ready
	}
}

UPBShotComponent* APBPlayerController::GetShotComponent() const
{
	const APBBallPawn* Ball = Cast<APBBallPawn>(GetPawn());
	return Ball ? Ball->GetShotComponent() : nullptr;
}

bool APBPlayerController::GetCursorScreenPos(FVector2D& OutPos) const
{
	float X = 0.f, Y = 0.f;
	if (GetMousePosition(X, Y)) // const in UE 5.7
	{
		OutPos = FVector2D(X, Y);
		return true;
	}
	return false;
}
