// Copyright Epic Games, Inc. All Rights Reserved.

#include "PBPlayerController.h"

#include "Ball/PBBallPawn.h"
#include "Camera/PBCameraSubsystem.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/World.h"
#include "Match/PBLobbyGameMode.h"
#include "Match/PBPlayerState.h"
#include "Shot/PBShotComponent.h"

APBPlayerController::APBPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
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
	}
}

void APBPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// Stream the live cursor position to the shot component while dragging so the
	// aim + power preview tracks the mouse every frame.
	if (bDragging)
	{
		if (UPBShotComponent* Shot = GetShotComponent())
		{
			FVector2D Cursor;
			if (GetCursorScreenPos(Cursor))
			{
				Shot->UpdateAim(Cursor);
			}
		}
	}
}

void APBPlayerController::OnDragStarted(const FInputActionValue& Value)
{
	UPBShotComponent* Shot = GetShotComponent();
	FVector2D Cursor;
	if (Shot && GetCursorScreenPos(Cursor))
	{
		bDragging = true;
		Shot->StartAim(Cursor);
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
	if (UPBCameraSubsystem* Cameras = GetWorld() ? GetWorld()->GetSubsystem<UPBCameraSubsystem>() : nullptr)
	{
		Cameras->CycleCamera(this);
	}
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
