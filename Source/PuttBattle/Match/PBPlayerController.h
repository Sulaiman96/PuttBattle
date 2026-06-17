// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Match/PBMatchTypes.h"
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

	/**
	 * Server seam: this player finished the hole → drop into spectate (T3.3). The
	 * real spectator system (free-switch between players, env powerups) is Phase 8;
	 * this just notifies the owning client so it can hold a placeholder camera. Kept
	 * as the single entry point so Phase 8 swaps the body, not the callers.
	 */
	void EnterSpectate();

	/** Server seam: leave spectate (called at each hole start so a player who sank
	 *  the previous hole returns to play). Mirrors EnterSpectate; Phase 8 swaps bodies. */
	void ExitSpectate();

	/** True once this client has entered the (placeholder) spectate state. */
	UFUNCTION(BlueprintPure, Category = "PB|Spectate")
	bool IsSpectating() const { return bSpectating; }

	// --- Lobby (T3.4; the lobby UMG calls these on the owning client) ------

	/** Toggle this player's lobby ready flag (server-authoritative). */
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "PB|Lobby")
	void Server_SetReady(bool bReady);

	/** Host-only: push the previewed match settings (the GameMode re-checks host + clamps). */
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "PB|Lobby")
	void Server_SetMatchSettings(FPBMatchSettings NewSettings);

	/** Host-only: request the match start (the GameMode re-checks host + all-ready). */
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "PB|Lobby")
	void Server_RequestStartMatch();

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

	/** Owning-client notification that spectate has begun (placeholder, Phase 8). */
	UFUNCTION(Client, Reliable)
	void Client_EnterSpectate();

	/** Owning-client notification that spectate has ended (new hole). */
	UFUNCTION(Client, Reliable)
	void Client_ExitSpectate();

	/** BP hook to swap to a placeholder fixed camera while spectating (Phase 8 real). */
	UFUNCTION(BlueprintImplementableEvent, Category = "PB|Spectate")
	void OnEnterSpectate();

	/** BP hook to restore the gameplay camera when spectate ends (Phase 8 real). */
	UFUNCTION(BlueprintImplementableEvent, Category = "PB|Spectate")
	void OnExitSpectate();

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

	/** Local-only spectate flag (set by Client_EnterSpectate). */
	bool bSpectating = false;
};
