// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PBPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UPBShotComponent;
struct FInputActionValue;

/**
 * One player's input and will (LEARNING.md framework quartet). In Phase 1 its
 * whole job is to drive the shot loop and cycle cameras: it captures the LMB
 * drag in screen space and feeds it to the possessed ball's UPBShotComponent,
 * which owns all the shot math. Camera cycling is client-only (no replication).
 *
 * Enhanced Input assets (IMC_Putt, IA_*) are wired on a BP subclass so designers
 * can rebind without code (LEARNING.md: IMC/IA). C++ only binds the callbacks.
 */
UCLASS()
class PUTTBATTLE_API APBPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	APBPlayerController();

	virtual void PlayerTick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	/** Drag-shot input (LMB press/hold/release). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PB|Input")
	TObjectPtr<UInputMappingContext> PuttMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PB|Input")
	int32 MappingContextPriority = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PB|Input")
	TObjectPtr<UInputAction> DragShotAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PB|Input")
	TObjectPtr<UInputAction> CycleCameraAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PB|Input")
	TObjectPtr<UInputAction> CancelAction;

private:
	void OnDragStarted(const FInputActionValue& Value);
	void OnDragReleased(const FInputActionValue& Value);
	void OnCycleCamera(const FInputActionValue& Value);
	void OnCancel(const FInputActionValue& Value);

	UPBShotComponent* GetShotComponent() const;

	/** Current cursor position in screen pixels, or false if unavailable. */
	bool GetCursorScreenPos(FVector2D& OutPos) const;

	/** True between LMB-down and LMB-up so PlayerTick streams drag updates. */
	bool bDragging = false;
};
