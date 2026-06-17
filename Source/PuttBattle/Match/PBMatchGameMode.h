// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Match/PBGameMode.h"
#include "Engine/TimerHandle.h"
#include "PBMatchGameMode.generated.h"

class APBMatchGameState;

/**
 * The multiplayer match referee (plans/03 T3.2). Owns every phase transition
 * server-side and writes them to APBMatchGameState; clients only read the phase.
 * Phase 3 lands the working loop HoleIntro → Countdown → Playing → (all sink) →
 * HoleResults → next hole / MatchResults. The real timer model (map clock,
 * first-finish clamp, ShotGrace) is Phase 4 — the Playing-timeout here is a
 * deliberate stub so the loop never hangs in testing.
 *
 * Phase durations are EditAnywhere so they stay tunable without a recompile
 * (CONVENTIONS §1); the defaults are the CONVENTIONS §7 reference values.
 * bUseSeamlessTravel is on so the lobby→match hop (T3.4) keeps players connected.
 */
UCLASS()
class PUTTBATTLE_API APBMatchGameMode : public APBGameMode
{
	GENERATED_BODY()

public:
	APBMatchGameMode();

	virtual void BeginPlay() override;

	/** Cup acceptance (server): stamp finish order/time, spectate, maybe end hole. */
	virtual void NotifyBallSunk(APBBallPawn* Ball) override;

	// --- Phase durations (feel/tuning surface, CONVENTIONS §7) -------------

	/** Grace after BeginPlay before the first hole, so all clients are connected. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Match|Timing", meta = (ClampMin = "0.0"))
	float WarmupSeconds = 2.f;

	/** Camera glance over the hole before the countdown (D20). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Match|Timing", meta = (ClampMin = "0.0"))
	float HoleIntroSeconds = 4.f;

	/** 3-2-1 countdown before control unlocks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Match|Timing", meta = (ClampMin = "0.0"))
	float CountdownSeconds = 3.f;

	/** Per-hole results display before the next hole / podium. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Match|Timing", meta = (ClampMin = "0.0"))
	float HoleResultsSeconds = 8.f;

protected:
	/** Begin the match (warmup elapsed): start the first hole. */
	void StartMatch();

	/** Enter HoleIntro for CurrentHoleIndex: reset every player to the tee. */
	void StartHole();

	void EnterCountdown();
	void EnterPlaying();

	/** Playing map-timer elapsed (Phase 3 stub for Phase 4's ShotGrace path). */
	void OnPlayingExpired();

	void EnterHoleResults();

	/** After HoleResults: next hole, or MatchResults once HoleCount is reached. */
	void AdvanceFromHoleResults();

	void EnterMatchResults();

private:
	APBMatchGameState* GetMatchGameState() const;

	/** Teleport every player to the tee and clear their per-hole PlayerState. */
	void ResetAllPlayersForHole();

	/** Drives every timed phase transition (only one is ever pending). */
	FTimerHandle PhaseTimer;

	/** Server-side finish counter for this hole (assigns FinishOrder). */
	int32 NumFinishedThisHole = 0;
};
