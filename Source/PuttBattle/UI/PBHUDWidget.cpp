// Copyright Epic Games, Inc. All Rights Reserved.

#include "PBHUDWidget.h"

#include "Ball/PBBallPawn.h"
#include "Course/PBCupActor.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Match/PBPlayerState.h"
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
	BindToPlayerState();

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
	UnbindFromPlayerState();
	if (APBCupActor* Cup = BoundCup.Get())
	{
		Cup->OnBallSunk.RemoveDynamic(this, &UPBHUDWidget::HandleBallSunk);
	}

	Super::NativeDestruct();
}

void UPBHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// On a remote client the local APBPlayerState often replicates AFTER possession,
	// so the construct/possession-time bind misses it and there's no later trigger.
	// Poll until bound (BindToPlayerState self-guards and seeds the current value);
	// this also re-binds if the PlayerState is swapped out by seamless travel.
	if (!BoundPlayerState.IsValid())
	{
		BindToPlayerState();
	}

	// Belt-and-suspenders: push the authoritative stroke count every frame so the
	// counter stays correct even if the delegate bind was missed by possession/owner
	// timing. Cheap — one text set on a single label.
	if (const APBPlayerState* PS = GetLocalPlayerState())
	{
		OnStrokesChanged(PS->GetStrokes());
	}
}

void UPBHUDWidget::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	BindToBall();
	BindToPlayerState();
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
	Shot->OnAimUpdated.AddDynamic(this, &UPBHUDWidget::HandleAimUpdated);
	Shot->OnAimEnded.AddDynamic(this, &UPBHUDWidget::HandleAimEnded);
}

void UPBHUDWidget::UnbindFromBall()
{
	if (UPBShotComponent* Shot = BoundShot.Get())
	{
		Shot->OnAimUpdated.RemoveDynamic(this, &UPBHUDWidget::HandleAimUpdated);
		Shot->OnAimEnded.RemoveDynamic(this, &UPBHUDWidget::HandleAimEnded);
	}
	BoundShot = nullptr;
}

void UPBHUDWidget::BindToPlayerState()
{
	APBPlayerState* PS = GetLocalPlayerState();
	if (!PS || PS == BoundPlayerState.Get())
	{
		return; // not ready yet, or already bound to this state
	}

	UnbindFromPlayerState();
	BoundPlayerState = PS;
	PS->OnStrokesChanged.AddDynamic(this, &UPBHUDWidget::HandleStrokesChanged);
	OnStrokesChanged(PS->GetStrokes()); // seed the current value
}

void UPBHUDWidget::UnbindFromPlayerState()
{
	if (APBPlayerState* PS = BoundPlayerState.Get())
	{
		PS->OnStrokesChanged.RemoveDynamic(this, &UPBHUDWidget::HandleStrokesChanged);
	}
	BoundPlayerState = nullptr;
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
	// Prefer the owning player, fall back to the first local controller: the HUD's
	// owning-player context can be unset depending on how it was created, which would
	// otherwise null the ball lookup (no sunk toast). Single-player: first PC is right.
	const APlayerController* PC = GetOwningPlayer();
	if (!PC && GetWorld())
	{
		PC = GetWorld()->GetFirstPlayerController();
	}
	return PC ? Cast<APBBallPawn>(PC->GetPawn()) : nullptr;
}

APBPlayerState* UPBHUDWidget::GetLocalPlayerState() const
{
	const APlayerController* PC = GetOwningPlayer();
	if (!PC && GetWorld())
	{
		PC = GetWorld()->GetFirstPlayerController(); // same owner fallback as GetLocalBall
	}
	return PC ? PC->GetPlayerState<APBPlayerState>() : nullptr;
}
