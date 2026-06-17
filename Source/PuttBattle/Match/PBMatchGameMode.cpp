// Copyright Epic Games, Inc. All Rights Reserved.

#include "PBMatchGameMode.h"

#include "Ball/PBBallPawn.h"
#include "Engine/World.h"
#include "Match/PBMatchGameState.h"
#include "Match/PBPlayerController.h"
#include "Match/PBPlayerState.h"
#include "Net/PBSessionSubsystem.h"
#include "TimerManager.h"

APBMatchGameMode::APBMatchGameMode()
{
	GameStateClass = APBMatchGameState::StaticClass();
	// Keep clients connected across the lobby→match (and future hole→hole) hop;
	// the transition map is a project setting (see DefaultEngine.ini / editor step).
	bUseSeamlessTravel = true;
}

void APBMatchGameMode::BeginPlay()
{
	Super::BeginPlay();

	// The phase machine is server-only. The warmup gives clients (PIE or seamless
	// travel) time to finish connecting before the first HoleIntro.
	if (HasAuthority())
	{
		if (WarmupSeconds > 0.f)
		{
			GetWorldTimerManager().SetTimer(PhaseTimer, this, &APBMatchGameMode::StartMatch, WarmupSeconds, false);
		}
		else
		{
			StartMatch();
		}
	}
}

void APBMatchGameMode::StartMatch()
{
	APBMatchGameState* GS = GetMatchGameState();
	if (!GS)
	{
		return;
	}

	// Adopt the host's lobby settings if a session is in flight; else defaults.
	FPBMatchSettings Settings;
	if (const UGameInstance* GI = GetGameInstance())
	{
		if (const UPBSessionSubsystem* Sess = GI->GetSubsystem<UPBSessionSubsystem>())
		{
			Settings = Sess->GetPendingMatchSettings();
		}
	}
	GS->SetMatchSettings(Settings);
	GS->SetCurrentHoleIndex(0);

	StartHole();
}

void APBMatchGameMode::StartHole()
{
	APBMatchGameState* GS = GetMatchGameState();
	if (!GS)
	{
		return;
	}

	NumFinishedThisHole = 0;
	ResetAllPlayersForHole();

	// HoleIntro: a timed glance, then the countdown (D20).
	GS->SetMatchPhase(EPBMatchPhase::HoleIntro, HoleIntroSeconds);
	GetWorldTimerManager().SetTimer(PhaseTimer, this, &APBMatchGameMode::EnterCountdown, FMath::Max(HoleIntroSeconds, 0.01f), false);
}

void APBMatchGameMode::EnterCountdown()
{
	if (APBMatchGameState* GS = GetMatchGameState())
	{
		GS->SetMatchPhase(EPBMatchPhase::Countdown, CountdownSeconds);
		GetWorldTimerManager().SetTimer(PhaseTimer, this, &APBMatchGameMode::EnterPlaying, FMath::Max(CountdownSeconds, 0.01f), false);
	}
}

void APBMatchGameMode::EnterPlaying()
{
	if (APBMatchGameState* GS = GetMatchGameState())
	{
		// The map clock is shown via PhaseEndServerTime; the real expiry→ShotGrace
		// rules are Phase 4. The timer here is only a hang-guard fallback so the
		// loop always advances if a ball can't be sunk.
		const float MapTime = GS->GetMatchSettings().MapTimeSeconds;
		GS->SetMatchPhase(EPBMatchPhase::Playing, MapTime);
		GetWorldTimerManager().SetTimer(PhaseTimer, this, &APBMatchGameMode::OnPlayingExpired, FMath::Max(MapTime, 0.01f), false);
	}
}

void APBMatchGameMode::OnPlayingExpired()
{
	// Phase 3 stub: time ran out. Players still at rest are implicitly DNF (D10);
	// Phase 4 replaces this with the ShotGrace simulation window.
	EnterHoleResults();
}

void APBMatchGameMode::EnterHoleResults()
{
	if (APBMatchGameState* GS = GetMatchGameState())
	{
		GetWorldTimerManager().ClearTimer(PhaseTimer);
		GS->SetMatchPhase(EPBMatchPhase::HoleResults, HoleResultsSeconds);
		GetWorldTimerManager().SetTimer(PhaseTimer, this, &APBMatchGameMode::AdvanceFromHoleResults, FMath::Max(HoleResultsSeconds, 0.01f), false);
	}
}

void APBMatchGameMode::AdvanceFromHoleResults()
{
	APBMatchGameState* GS = GetMatchGameState();
	if (!GS)
	{
		return;
	}

	const int32 NextHole = GS->GetCurrentHoleIndex() + 1;
	if (NextHole < GS->GetMatchSettings().HoleCount)
	{
		// Phase 3 loops the same graybox hole (real variant streaming is Phase 7).
		GS->SetCurrentHoleIndex(NextHole);
		StartHole();
	}
	else
	{
		EnterMatchResults();
	}
}

void APBMatchGameMode::EnterMatchResults()
{
	if (APBMatchGameState* GS = GetMatchGameState())
	{
		GetWorldTimerManager().ClearTimer(PhaseTimer);
		// Terminal for the match; podium → lobby return is Phase 4 / T3.4.
		GS->SetMatchPhase(EPBMatchPhase::MatchResults, 0.f);
	}
}

void APBMatchGameMode::NotifyBallSunk(APBBallPawn* Ball)
{
	if (!HasAuthority() || !Ball)
	{
		return;
	}

	APBMatchGameState* GS = GetMatchGameState();
	APBPlayerState* PS = Ball->GetPlayerState<APBPlayerState>();
	if (!GS || !PS || PS->IsFinished())
	{
		return;
	}

	// Server-stamped order + time is the authoritative sink ordering (T3.3) — never
	// the client's clock.
	const int32 Order = NumFinishedThisHole++;
	PS->MarkFinished(Order, GS->GetServerWorldTimeSeconds());

	// Finished player drops into the placeholder spectate seam (real one is Phase 8).
	if (APBPlayerController* PC = Cast<APBPlayerController>(Ball->GetController()))
	{
		PC->EnterSpectate();
	}

	// Everyone in → end the hole immediately (DF7), pre-empting the map clock.
	if (GS->GetMatchPhase() == EPBMatchPhase::Playing && GS->AreAllPlayersFinished())
	{
		EnterHoleResults();
	}
}

APBMatchGameState* APBMatchGameMode::GetMatchGameState() const
{
	return GetWorld() ? GetWorld()->GetGameState<APBMatchGameState>() : nullptr;
}

void APBMatchGameMode::ResetAllPlayersForHole()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		// RestartHole teleports the ball to the tee and clears the PlayerState's
		// per-hole fields (strokes / checkpoint / finish).
		APlayerController* PC = It->Get();
		RestartHole(PC);

		// A player who sank the previous hole is in the placeholder spectate state —
		// pull them back out so they can play this hole (mirrors the finish→spectate
		// seam; no-op if they weren't spectating).
		if (APBPlayerController* PBPC = Cast<APBPlayerController>(PC))
		{
			PBPC->ExitSpectate();
		}
	}
}
