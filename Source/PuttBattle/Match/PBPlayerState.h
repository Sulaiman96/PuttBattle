// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "PBPlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPBStrokesChanged, int32, NewStrokes);

/**
 * Per-player replicated facts (LEARNING.md: the framework quartet). Phase 3 lands
 * the fields the match flow needs: strokes this hole (the authoritative count —
 * moved off UPBShotComponent because the shot now executes server-side, so a
 * client's local component never increments), a placeholder total score (Phase 4
 * scoring fills it), the latest activated checkpoint index (D7 respawn), and the
 * finish state + server-stamped sink order/time (T3.3: order must be server truth,
 * not client-timed).
 *
 * Every mutator here is server-only (authority); clients receive the values via
 * replication and OnRep_* only updates presentation (CONVENTIONS §2). Custom
 * fields ride across seamless travel via CopyProperties (the lobby→match hop).
 */
UCLASS()
class PUTTBATTLE_API APBPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	APBPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Carry custom per-player fields across seamless travel (lobby→match). The
	 *  engine calls this on the OLD state with the NEW one as the argument. */
	virtual void CopyProperties(APlayerState* NewPlayerState) override;

	// --- Reads (UI / scoreboard) -------------------------------------------

	UFUNCTION(BlueprintPure, Category = "PB|Match")
	int32 GetStrokes() const { return Strokes; }

	UFUNCTION(BlueprintPure, Category = "PB|Match")
	int32 GetTotalScore() const { return TotalScore; }

	UFUNCTION(BlueprintPure, Category = "PB|Match")
	int32 GetCheckpointIndex() const { return CheckpointIndex; }

	UFUNCTION(BlueprintPure, Category = "PB|Match")
	bool IsFinished() const { return bFinished; }

	UFUNCTION(BlueprintPure, Category = "PB|Match")
	int32 GetFinishOrder() const { return FinishOrder; }

	UFUNCTION(BlueprintPure, Category = "PB|Match")
	double GetFinishServerTime() const { return FinishServerTime; }

	/** Lobby ready-up state (T3.4). Lives here to avoid a separate lobby PlayerState. */
	UFUNCTION(BlueprintPure, Category = "PB|Match")
	bool IsReady() const { return bReady; }

	// --- Server mutators (authority only) ----------------------------------

	/** Set the lobby ready flag (server). */
	void SetReady(bool bInReady);

	/** +1 stroke this hole (called from the server shot funnel). */
	void AddStroke();

	/** Reset per-hole state at the start of a new hole (strokes, finish, checkpoint). */
	void ResetForNewHole();

	/** Record the highest activated checkpoint (never regresses, D7). */
	void SetCheckpointIndex(int32 NewIndex);

	/** Mark this player finished with a server-assigned order + server timestamp. */
	void MarkFinished(int32 Order, double ServerTime);

	/** Fired on the owning client when the stroke count replicates (HUD binds here). */
	UPROPERTY(BlueprintAssignable, Category = "PB|Match")
	FPBStrokesChanged OnStrokesChanged;

protected:
	UFUNCTION()
	void OnRep_Strokes();

	UFUNCTION()
	void OnRep_Finished();

	/** Presentation hook: this player just finished the hole (BP toast / camera). */
	UFUNCTION(BlueprintImplementableEvent, Category = "PB|Match")
	void OnFinishedChanged(bool bNowFinished);

private:
	/** Strokes taken on the current hole. */
	UPROPERTY(ReplicatedUsing = OnRep_Strokes, Transient)
	int32 Strokes = 0;

	/** Running match score (placeholder until Phase 4 scoring). */
	UPROPERTY(Replicated, Transient)
	int32 TotalScore = 0;

	/** Highest activated checkpoint this hole; INDEX_NONE = tee (D7). */
	UPROPERTY(Replicated, Transient)
	int32 CheckpointIndex = INDEX_NONE;

	/** True once this player has sunk this hole. */
	UPROPERTY(ReplicatedUsing = OnRep_Finished, Transient)
	bool bFinished = false;

	/** Finish placement this hole (0 = first to sink); INDEX_NONE until finished. */
	UPROPERTY(Replicated, Transient)
	int32 FinishOrder = INDEX_NONE;

	/** Server world time of the sink — the authoritative ordering key (T3.3). */
	UPROPERTY(Replicated, Transient)
	double FinishServerTime = 0.0;

	/** Lobby ready-up flag (T3.4). */
	UPROPERTY(Replicated, Transient)
	bool bReady = false;
};
