// Copyright Epic Games, Inc. All Rights Reserved.

#include "PBHUDWidget.h"

#include "Ball/PBBallPawn.h"
#include "Course/PBCupActor.h"
#include "EngineUtils.h"
#include "Shot/PBShotComponent.h"

void UPBHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (APBBallPawn* Ball = GetLocalBall())
	{
		if (UPBShotComponent* Shot = Ball->GetShotComponent())
		{
			BoundShot = Shot;
			Shot->OnStrokeCountChanged.AddDynamic(this, &UPBHUDWidget::HandleStrokesChanged);
			Shot->OnAimUpdated.AddDynamic(this, &UPBHUDWidget::HandleAimUpdated);
			Shot->OnAimEnded.AddDynamic(this, &UPBHUDWidget::HandleAimEnded);
			OnStrokesChanged(Shot->GetStrokeCount());
		}
	}

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
	if (UPBShotComponent* Shot = BoundShot.Get())
	{
		Shot->OnStrokeCountChanged.RemoveDynamic(this, &UPBHUDWidget::HandleStrokesChanged);
		Shot->OnAimUpdated.RemoveDynamic(this, &UPBHUDWidget::HandleAimUpdated);
		Shot->OnAimEnded.RemoveDynamic(this, &UPBHUDWidget::HandleAimEnded);
	}
	if (APBCupActor* Cup = BoundCup.Get())
	{
		Cup->OnBallSunk.RemoveDynamic(this, &UPBHUDWidget::HandleBallSunk);
	}

	Super::NativeDestruct();
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
