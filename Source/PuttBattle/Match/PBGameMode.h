// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PBGameMode.generated.h"

class APBBallPawn;
class APBCheckpointActor;
class APBTeePad;

/**
 * The base PB referee. It owns the player-agnostic course rules every mode
 * shares: respawn-at-checkpoint (D7), checkpoint tracking, and hole restart.
 * Used directly for offline feel-testing (Phases 1–2); APBMatchGameMode extends
 * it with the multiplayer phase machine, scoring, and seamless travel (Phase 3+).
 *
 * Checkpoint state lives on APBPlayerState (CheckpointIndex), not on the pawn —
 * it must be server-authoritative and survive re-possession/travel (T3.3). This
 * mode sets PlayerStateClass so even offline play carries one PB PlayerState.
 */
UCLASS()
class PUTTBATTLE_API APBGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	APBGameMode();

	/** Respawn the controller's ball at the tee and reset its per-hole state. */
	UFUNCTION(BlueprintCallable, Category = "PB|Match")
	void RestartHole(APlayerController* PC);

	/** Record a ball reaching a checkpoint on its PlayerState (keeps the highest, D7). */
	void ActivateCheckpoint(APBBallPawn* Ball, APBCheckpointActor* Checkpoint);

	/**
	 * Teleport the ball to its PlayerState's latest activated checkpoint (tee if
	 * none), zero velocity, no stroke change (D7). The single respawn path for
	 * water, void, and KillZ falls.
	 */
	UFUNCTION(BlueprintCallable, Category = "PB|Match")
	void RespawnAtCheckpoint(APBBallPawn* Ball);

	/**
	 * A ball was accepted by a cup (server). Base mode does nothing (offline has
	 * no match flow); APBMatchGameMode records finish order/time + spectate.
	 */
	virtual void NotifyBallSunk(APBBallPawn* Ball) {}

protected:
	/** Find the first tee pad in the level (Phase 1: one hole, one tee). */
	APBTeePad* FindTeePad() const;

	/** The placed checkpoint whose CheckpointIndex matches (nullptr for INDEX_NONE). */
	APBCheckpointActor* FindCheckpointByIndex(int32 Index) const;
};
