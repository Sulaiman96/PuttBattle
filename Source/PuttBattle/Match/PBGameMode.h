// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PBGameMode.generated.h"

class APBBallPawn;
class APBCheckpointActor;
class APBTeePad;

/**
 * Phase 1 offline referee: just wires the ball pawn + PB player controller and
 * provides a hole-restart that respawns the ball at the tee. The full phase
 * machine (HoleIntro → Countdown → Playing → … CONVENTIONS §7) lands in Phase 4;
 * this stays deliberately thin so it can be replaced without churn.
 */
UCLASS()
class PUTTBATTLE_API APBGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	APBGameMode();

	/** Respawn the controller's ball at the tee pad and reset its strokes. */
	UFUNCTION(BlueprintCallable, Category = "PB|Match")
	void RestartHole(APlayerController* PC);

	/** Record a ball reaching a checkpoint; keeps only its highest index (D7). */
	void ActivateCheckpoint(APBBallPawn* Ball, APBCheckpointActor* Checkpoint);

	/**
	 * Teleport the ball to its latest activated checkpoint (tee if none), zero
	 * velocity, no stroke change (D7). The single respawn path for water, void,
	 * and KillZ falls.
	 */
	UFUNCTION(BlueprintCallable, Category = "PB|Match")
	void RespawnAtCheckpoint(APBBallPawn* Ball);

protected:
	/** Find the first tee pad in the level (Phase 1: one hole, one tee). */
	APBTeePad* FindTeePad() const;

private:
	/** Each ball's highest-activated checkpoint (per PlayerState in Phase 3+). */
	UPROPERTY(Transient)
	TMap<TObjectPtr<APBBallPawn>, TObjectPtr<APBCheckpointActor>> LatestCheckpoint;
};
