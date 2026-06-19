// Copyright Epic Games, Inc. All Rights Reserved.

#include "PBLobbyGameMode.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Match/PBLobbyGameState.h"
#include "Match/PBPlayerController.h"
#include "Match/PBPlayerState.h"
#include "Net/PBSessionSubsystem.h"

APBLobbyGameMode::APBLobbyGameMode()
{
	GameStateClass = APBLobbyGameState::StaticClass();
	PlayerStateClass = APBPlayerState::StaticClass();
	PlayerControllerClass = APBPlayerController::StaticClass();
	DefaultPawnClass = nullptr; // no ball in the lobby; players are UI-only
	bUseSeamlessTravel = true;  // keep everyone connected across the hop into the match
}

void APBLobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// First to arrive on the listen server is the host — the settings/start authority.
	if (!HostController)
	{
		HostController = NewPlayer;
	}
}

void APBLobbyGameMode::HostSetMatchSettings(APlayerController* By, const FPBMatchSettings& NewSettings)
{
	if (By != HostController)
	{
		return; // host-only settings panel (T3.4)
	}
	if (APBLobbyGameState* GS = GetGameState<APBLobbyGameState>())
	{
		// Clamp into the design ranges (D11 / DF1) regardless of what the client sent.
		FPBMatchSettings Clamped = NewSettings;
		Clamped.HoleCount = FMath::Clamp(Clamped.HoleCount, 3, 9);
		Clamped.MapTimeSeconds = FMath::Clamp(Clamped.MapTimeSeconds, 60.f, 300.f);
		Clamped.FirstFinishClampSeconds = FMath::Clamp(Clamped.FirstFinishClampSeconds, 10.f, 60.f);
		GS->SetMatchSettings(Clamped);
	}
}

void APBLobbyGameMode::RequestStartMatch(APlayerController* By)
{
	if (By != HostController)
	{
		return; // only the host starts the match
	}

	const APBLobbyGameState* GS = GetGameState<APBLobbyGameState>();
	if (!GS || !GS->AreAllPlayersReady())
	{
		return; // need ≥2 players, all ready (D16)
	}

	// Carry the host's settings forward — the lobby GameState won't survive travel,
	// but the GameInstance subsystem will, and the match GameMode reads it on start.
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UPBSessionSubsystem* Session = GI->GetSubsystem<UPBSessionSubsystem>())
		{
			Session->SetPendingMatchSettings(GS->GetMatchSettings());
		}
	}

	// Seamless travel keeps the listen server up and all PlayerControllers/States.
	// ?game= forces the match GameMode on the destination map (overrides its World
	// Settings, so the offline feel-test maps keep their own referee).
	if (UWorld* World = GetWorld())
	{
		World->ServerTravel(FString::Printf(TEXT("%s?game=%s?listen"), *MatchMapURL, *MatchGameModeURL));
	}
}
