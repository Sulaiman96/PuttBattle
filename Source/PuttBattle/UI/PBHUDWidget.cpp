// Copyright Epic Games, Inc. All Rights Reserved.

#include "PBHUDWidget.h"

#include "Ball/PBBallPawn.h"
#include "Course/PBCupActor.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Shot/PBShotComponent.h"

void UPBHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// The HUD is created from the controller's BeginPlay, which can run before the
	// ball is possessed; bind now if it's ready and again on every possession
	// change (also covers respawn re-possession in Phase 3).
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->OnPossessedPawnChanged.AddDynamic(this, &UPBHUDWidget::HandlePossessedPawnChanged);
	}
	BindToBall();

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<APBCupActor> It(World); It; ++It)
		{
			BoundCup = *It;
			It->OnBallSunk.AddDynamic(this, &UPBHUDWidget::HandleBallSunk);
			break; // Phase 1: one hole, one cup
		}
	}
}

void UPBHUDWidget::NativeDestruct()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->OnPossessedPawnChanged.RemoveDynamic(this, &UPBHUDWidget::HandlePossessedPawnChanged);
	}
	UnbindFromBall();
	if (APBCupActor* Cup = BoundCup.Get())
	{
		Cup->OnBallSunk.RemoveDynamic(this, &UPBHUDWidget::HandleBallSunk);
	}

	Super::NativeDestruct();
}

void UPBHUDWidget::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	BindToBall();
}

void UPBHUDWidget::BindToBall()
{
	APBBallPawn* Ball = GetLocalBall();
	UPBShotComponent* Shot = Ball ? Ball->GetShotComponent() : nullptr;
	if (!Shot || Shot == BoundShot.Get())
	{
		return; // nothing to bind, or already bound to this ball
	}

	UnbindFromBall();
	BoundShot = Shot;
	Shot->OnStrokeCountChanged.AddDynamic(this, &UPBHUDWidget::HandleStrokesChanged);
	Shot->OnAimUpdated.AddDynamic(this, &UPBHUDWidget::HandleAimUpdated);
	Shot->OnAimEnded.AddDynamic(this, &UPBHUDWidget::HandleAimEnded);
	OnStrokesChanged(Shot->GetStrokeCount());
}

void UPBHUDWidget::UnbindFromBall()
{
	if (UPBShotComponent* Shot = BoundShot.Get())
	{
		Shot->OnStrokeCountChanged.RemoveDynamic(this, &UPBHUDWidget::HandleStrokesChanged);
		Shot->OnAimUpdated.RemoveDynamic(this, &UPBHUDWidget::HandleAimUpdated);
		Shot->OnAimEnded.RemoveDynamic(this, &UPBHUDWidget::HandleAimEnded);
	}
	BoundShot = nullptr;
}

void UPBHUDWidget::HandleStrokesChanged(int32 NewStrokeCount)
{
	OnStrokesChanged(NewStrokeCount);
}

void UPBHUDWidget::HandleAimUpdated(float Power01, bool bIndicatorHidden)
{
	OnAimPower(Power01, bIndicatorHidden);
}

void UPBHUDWidget::HandleAimEnded()
{
	OnAimEnded();
}

void UPBHUDWidget::HandleBallSunk(APBBallPawn* Ball)
{
	if (Ball && Ball == GetLocalBall())
	{
		OnLocalBallSunk();
	}
}

APBBallPawn* UPBHUDWidget::GetLocalBall() const
{
	if (const APlayerController* PC = GetOwningPlayer())
	{
		return Cast<APBBallPawn>(PC->GetPawn());
	}
	return nullptr;
}
