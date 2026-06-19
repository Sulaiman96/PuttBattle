// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Match/PBMatchTypes.h"
#include "PBPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UPBShotComponent;
class APBCupActor;
class APBCameraRig;
struct FInputActionValue;

/**
 * One ball-follow camera framing (cam 1, cam 2). The spring-arm pitch/length/FOV
 * are applied to the possessed ball's CameraBoom when this preset is selected.
 * Feel values live here (EditAnywhere) so designers retune framing without code
 * (CONVENTIONS §1). Cam 3 (the map overview) is a separate auto-orbit APBCameraRig.
 */
USTRUCT(BlueprintType)
struct FPBFollowCamPreset
{
	GENERATED_BODY()

	/** Label for readability in the details panel (e.g. "Play", "Shot"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Camera")
	FName Label = NAME_None;

	/** Spring-arm pitch in degrees; negative looks down. Less steep = "angled up more". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Camera")
	float Pitch = -30.f;

	/** Spring-arm length (cm) — how far the camera sits from the ball. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Camera")
	float ArmLength = 650.f;

	/** Camera field of view (deg). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Camera")
	float FieldOfView = 75.f;

	/** When true the boom auto-yaws to keep the hole in front of the ball (cam 2:
	 *  frames ball + hole). Manual orbit is ignored for this preset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Camera")
	bool bFaceHole = false;

	/** When true, RMB-drag can orbit this framing (cam 1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Camera")
	bool bAllowOrbit = true;
};

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

	/** Direct camera selection (keys 1/2/3 → IA_Camera1/2/3). 1,2 = follow framings; 3 = map overview. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PB|Input")
	TObjectPtr<UInputAction> Camera1Action;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PB|Input")
	TObjectPtr<UInputAction> Camera2Action;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PB|Input")
	TObjectPtr<UInputAction> Camera3Action;

	/** Hold-to-orbit the play camera (RMB). Drag left/right while held to turn the view. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PB|Input")
	TObjectPtr<UInputAction> OrbitCameraAction;

	// --- Camera framings (CONVENTIONS §1 feel-tuning surface) --------------

	/** The ball-follow framings, in selection order. Index 0 = cam 1, index 1 = cam 2;
	 *  selecting the index past the last one switches to the map-overview rig (cam 3). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Camera")
	TArray<FPBFollowCamPreset> FollowCamPresets;

	/** Orbit sensitivity (boom yaw degrees per pixel of RMB drag). Mirrored live by
	 *  pb.Camera.OrbitSpeed (set that >= 0 to override this authored value in PIE). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Camera", meta = (ClampMin = "0.0"))
	float OrbitYawSpeed = 0.8f;

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

	// Camera input (1/2/3 select a framing, RMB-hold orbits the play camera).
	void OnSelectCamera1(const FInputActionValue& Value);
	void OnSelectCamera2(const FInputActionValue& Value);
	void OnSelectCamera3(const FInputActionValue& Value);
	void OnOrbitStarted(const FInputActionValue& Value);
	void OnOrbitReleased(const FInputActionValue& Value);

	/** Select a camera by index (0..N-1 = follow framings, N = map-overview rig). */
	void SelectCameraMode(int32 Index);
	/** Advance to the next camera (kept for an optional cycle key). */
	void CycleCameraMode();
	/** Push the active mode onto the boom/camera or swap to the overview rig. */
	void ApplyCameraMode();
	/** Per-frame camera upkeep: face-the-hole easing and RMB orbit. */
	void UpdateActiveCamera(float DeltaTime);

	/** The hole's cup (for cam 2 face-hole framing), found + cached on first use. */
	APBCupActor* GetCup();
	/** The level's auto-orbit overview rig (cam 3), found + cached on first use. */
	APBCameraRig* GetOrbitRig();

	UPBShotComponent* GetShotComponent() const;

	/** Current cursor position in screen pixels, or false if unavailable. */
	bool GetCursorScreenPos(FVector2D& OutPos) const;

	/** True between LMB-down and LMB-up so PlayerTick streams drag updates. */
	bool bDragging = false;

	/** True while RMB is held to orbit the play camera. */
	bool bOrbiting = false;

	/** Active camera selection (index into FollowCamPresets, or == Num() for the overview rig). */
	int32 CameraMode = 0;

	/** Local-only spectate flag (set by Client_EnterSpectate). */
	bool bSpectating = false;

	/** Lazily-cached scene lookups (camera is client-only presentation). */
	TWeakObjectPtr<APBCupActor> CachedCup;
	TWeakObjectPtr<APBCameraRig> CachedOrbitRig;
};
