// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PBHUDWidget.generated.h"

class APBBallPawn;
class APBCupActor;
class UPBShotComponent;

/**
 * Base for WBP_HUD (CONVENTIONS §1: C++ owns the data plumbing, the BP owns the
 * visuals). On construct it finds the local ball's shot component + the hole's
 * cup and binds their delegates, then forwards them to BlueprintImplementable
 * events the widget draws (stroke counter, power bar, "SUNK!" toast). The HUD
 * only renders state it is handed — it never computes game truth (CONVENTIONS §2).
 */
UCLASS()
class PUTTBATTLE_API UPBHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	/** Stroke count changed (also fired once on bind for the initial value). */
	UFUNCTION(BlueprintImplementableEvent, Category = "PB|HUD")
	void OnStrokesChanged(int32 NewStrokeCount);

	/** Aim power [0,1] while dragging; bIndicatorHidden when Jumbled. */
	UFUNCTION(BlueprintImplementableEvent, Category = "PB|HUD")
	void OnAimPower(float Power01, bool bIndicatorHidden);

	/** Aim ended (fired / cancelled) — hide the power bar. */
	UFUNCTION(BlueprintImplementableEvent, Category = "PB|HUD")
	void OnAimEnded();

	/** The local ball was sunk — show the toast. */
	UFUNCTION(BlueprintImplementableEvent, Category = "PB|HUD")
	void OnLocalBallSunk();

private:
	UFUNCTION()
	void HandleStrokesChanged(int32 NewStrokeCount);

	UFUNCTION()
	void HandleAimUpdated(float Power01, bool bIndicatorHidden);

	UFUNCTION()
	void HandleAimEnded();

	UFUNCTION()
	void HandleBallSunk(APBBallPawn* Ball);

	/** Re-bind when the controller possesses a (new) pawn — covers HUD-before-pawn
	 *  and respawn re-possession (Phase 3). */
	UFUNCTION()
	void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	/** (Re)bind the shot delegates to the locally-controlled ball, if any. */
	void BindToBall();
	void UnbindFromBall();

	APBBallPawn* GetLocalBall() const;

	UPROPERTY(Transient)
	TWeakObjectPtr<UPBShotComponent> BoundShot;

	UPROPERTY(Transient)
	TWeakObjectPtr<APBCupActor> BoundCup;
};
