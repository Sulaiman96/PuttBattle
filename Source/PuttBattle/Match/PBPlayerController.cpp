// Copyright Epic Games, Inc. All Rights Reserved.

#include "PBPlayerController.h"

#include "Ball/PBBallPawn.h"
#include "Camera/PBCameraSubsystem.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
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

UPBShotComponent* APBPlayerController::GetShotComponent() const
{
	const APBBallPawn* Ball = Cast<APBBallPawn>(GetPawn());
	return Ball ? Ball->GetShotComponent() : nullptr;
}

bool APBPlayerController::GetCursorScreenPos(FVector2D& OutPos) const
{
	float X = 0.f, Y = 0.f;
	if (const_cast<APBPlayerController*>(this)->GetMousePosition(X, Y))
	{
		OutPos = FVector2D(X, Y);
		return true;
	}
	return false;
}
