// Copyright Epic Games, Inc. All Rights Reserved.

#include "PBSessionSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"

namespace
{
	/** Custom advertised key carrying the host's display name into the browse list. */
	static const FName PBRoomNameKey(TEXT("PB_ROOMNAME"));
}

void UPBSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Bind the persistent invite/"Join Game" delegate at startup so an invite
	// accepted from the menu (before any room exists) is caught (5.7-verified path).
	if (const IOnlineSessionPtr Sessions = GetSessions())
	{
		InviteHandle = Sessions->AddOnSessionUserInviteAcceptedDelegate_Handle(
			FOnSessionUserInviteAcceptedDelegate::CreateUObject(this, &UPBSessionSubsystem::HandleInviteAccepted));
	}

	// Listen-server host disconnects → bounce clients to the menu (host-left, T3.4).
	if (GEngine)
	{
		NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(this, &UPBSessionSubsystem::HandleNetworkFailure);
	}
}

void UPBSessionSubsystem::Deinitialize()
{
	if (const IOnlineSessionPtr Sessions = GetSessions())
	{
		if (InviteHandle.IsValid())  { Sessions->ClearOnSessionUserInviteAcceptedDelegate_Handle(InviteHandle); }
		if (CreateHandle.IsValid())  { Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle); }
		if (FindHandle.IsValid())    { Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle); }
		if (JoinHandle.IsValid())    { Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle); }
		if (DestroyHandle.IsValid()) { Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle); }
	}
	if (GEngine && NetworkFailureHandle.IsValid())
	{
		GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
	}

	Super::Deinitialize();
}

IOnlineSessionPtr UPBSessionSubsystem::GetSessions() const
{
	IOnlineSubsystem* Sub = IOnlineSubsystem::Get();
	return Sub ? Sub->GetSessionInterface() : nullptr;
}

FUniqueNetIdPtr UPBSessionSubsystem::GetLocalUserId() const
{
	if (const UGameInstance* GI = GetGameInstance())
	{
		if (const ULocalPlayer* LP = GI->GetFirstGamePlayer())
		{
			return LP->GetPreferredUniqueNetId().GetUniqueNetId();
		}
	}
	return nullptr;
}

void UPBSessionSubsystem::CreateRoom(int32 MaxPlayers, const FString& RoomName)
{
	const IOnlineSessionPtr Sessions = GetSessions();
	const FUniqueNetIdPtr UserId = GetLocalUserId();
	if (!Sessions.IsValid() || !UserId.IsValid())
	{
		OnCreateRoomComplete.Broadcast(false);
		return;
	}

	PendingRoomName = RoomName;
	PendingMaxPlayers = MaxPlayers;

	// If a session already exists (prior match / aborted host), destroy it FIRST and
	// create only once the destroy completes — DestroySession is async (a Steam
	// round-trip), so a synchronous Create right after would fail (the named session
	// still exists). Chain via the destroy delegate.
	if (Sessions->GetNamedSession(NAME_GameSession))
	{
		bCreateAfterDestroy = true;
		if (DestroyHandle.IsValid())
		{
			Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
		}
		DestroyHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(
			FOnDestroySessionCompleteDelegate::CreateUObject(this, &UPBSessionSubsystem::HandleDestroyComplete));
		if (!Sessions->DestroySession(NAME_GameSession))
		{
			Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
			bCreateAfterDestroy = false;
			DoCreateSession(); // nothing to wait on; create now
		}
		return;
	}

	DoCreateSession();
}

void UPBSessionSubsystem::DoCreateSession()
{
	const IOnlineSessionPtr Sessions = GetSessions();
	const FUniqueNetIdPtr UserId = GetLocalUserId();
	if (!Sessions.IsValid() || !UserId.IsValid())
	{
		OnCreateRoomComplete.Broadcast(false);
		return;
	}

	TSharedRef<FOnlineSessionSettings> Settings = MakeShared<FOnlineSessionSettings>();
	Settings->NumPublicConnections = FMath::Clamp(PendingMaxPlayers, 2, 6); // lobby size 2–6 (D16)
	Settings->NumPrivateConnections = 0;
	Settings->bShouldAdvertise = true;
	Settings->bAllowJoinInProgress = false;     // v1: join in lobby only (T3.4 robustness)
	Settings->bIsLANMatch = false;
	// Steam treats presence == lobby; bUsesPresence and bUseLobbiesIfAvailable MUST
	// agree or the join silently breaks (5.7-verified). Set the full friend-joinable set.
	Settings->bUsesPresence = true;
	Settings->bUseLobbiesIfAvailable = true;
	Settings->bAllowJoinViaPresence = true;
	Settings->bAllowInvites = true;
	Settings->bUsesStats = false;
	Settings->Set(SETTING_MAPNAME, LobbyMapURL, EOnlineDataAdvertisementType::ViaOnlineService);
	Settings->Set(PBRoomNameKey, PendingRoomName, EOnlineDataAdvertisementType::ViaOnlineService);

	// Clear a stale handle first so a re-issued create can't orphan a prior binding.
	if (CreateHandle.IsValid())
	{
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
	}
	CreateHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &UPBSessionSubsystem::HandleCreateComplete));

	if (!Sessions->CreateSession(*UserId, NAME_GameSession, *Settings))
	{
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
		OnCreateRoomComplete.Broadcast(false);
	}
}

void UPBSessionSubsystem::HandleCreateComplete(FName SessionName, bool bWasSuccessful)
{
	if (const IOnlineSessionPtr Sessions = GetSessions())
	{
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
	}
	OnCreateRoomComplete.Broadcast(bWasSuccessful);

	// Host opens the lobby map as a listen server; the session now advertises it.
	// The lobby GameMode comes from L_Lobby's own World Settings (set to a
	// PBLobbyGameMode that shows the lobby UI), so we do NOT force ?game= here —
	// that keeps the host and joining clients on the same lobby mode + HUD. (The
	// match hop still forces ?game= in APBLobbyGameMode::RequestStartMatch, which is
	// what keeps the offline feel-test map V_A on its own referee.)
	if (bWasSuccessful)
	{
		if (UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
		{
			World->ServerTravel(LobbyMapURL + TEXT("?listen"));
		}
	}
}

void UPBSessionSubsystem::FindRooms(int32 MaxResults)
{
	const IOnlineSessionPtr Sessions = GetSessions();
	const FUniqueNetIdPtr UserId = GetLocalUserId();
	if (!Sessions.IsValid() || !UserId.IsValid())
	{
		OnFindRoomsComplete.Broadcast(0);
		return;
	}

	SearchSettings = MakeShared<FOnlineSessionSearch>();
	SearchSettings->MaxSearchResults = FMath::Max(1, MaxResults);
	SearchSettings->bIsLanQuery = false;
	// Steam lobby search (SEARCH_PRESENCE was removed in 5.7 — SEARCH_LOBBIES is it).
	SearchSettings->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);

	// Clear a stale handle so a re-issued search (spammy browse UI) can't orphan it.
	if (FindHandle.IsValid())
	{
		Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
	}
	FindHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &UPBSessionSubsystem::HandleFindComplete));

	if (!Sessions->FindSessions(*UserId, SearchSettings.ToSharedRef()))
	{
		Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
		OnFindRoomsComplete.Broadcast(0);
	}
}

void UPBSessionSubsystem::HandleFindComplete(bool bWasSuccessful)
{
	if (const IOnlineSessionPtr Sessions = GetSessions())
	{
		Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
	}
	const int32 Num = (bWasSuccessful && SearchSettings.IsValid()) ? SearchSettings->SearchResults.Num() : 0;
	OnFindRoomsComplete.Broadcast(Num);
}

void UPBSessionSubsystem::JoinRoomByIndex(int32 SearchResultIndex)
{
	const IOnlineSessionPtr Sessions = GetSessions();
	const FUniqueNetIdPtr UserId = GetLocalUserId();
	if (!Sessions.IsValid() || !UserId.IsValid()
		|| !SearchSettings.IsValid() || !SearchSettings->SearchResults.IsValidIndex(SearchResultIndex))
	{
		OnJoinRoomComplete.Broadcast(false);
		return;
	}

	if (JoinHandle.IsValid())
	{
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
	}
	JoinHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &UPBSessionSubsystem::HandleJoinComplete));

	if (!Sessions->JoinSession(*UserId, NAME_GameSession, SearchSettings->SearchResults[SearchResultIndex]))
	{
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
		OnJoinRoomComplete.Broadcast(false);
	}
}

void UPBSessionSubsystem::HandleJoinComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (const IOnlineSessionPtr Sessions = GetSessions())
	{
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
	}

	const bool bOk = Result == EOnJoinSessionCompleteResult::Success
		|| Result == EOnJoinSessionCompleteResult::AlreadyInSession;
	OnJoinRoomComplete.Broadcast(bOk);

	if (bOk)
	{
		TravelToResolvedSession(); // JoinSession does NOT travel — we do it here
	}
}

void UPBSessionSubsystem::TravelToResolvedSession()
{
	const IOnlineSessionPtr Sessions = GetSessions();
	if (!Sessions.IsValid())
	{
		return;
	}

	FString ConnectString;
	if (Sessions->GetResolvedConnectString(NAME_GameSession, ConnectString))
	{
		if (APlayerController* PC = GetGameInstance() ? GetGameInstance()->GetFirstLocalPlayerController() : nullptr)
		{
			PC->ClientTravel(ConnectString, TRAVEL_Absolute);
		}
	}
}

void UPBSessionSubsystem::DestroyRoom()
{
	const IOnlineSessionPtr Sessions = GetSessions();
	if (!Sessions.IsValid())
	{
		OnDestroyRoomComplete.Broadcast(false);
		return;
	}

	DestroyHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &UPBSessionSubsystem::HandleDestroyComplete));

	if (!Sessions->DestroySession(NAME_GameSession))
	{
		Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
		OnDestroyRoomComplete.Broadcast(false);
	}
}

void UPBSessionSubsystem::HandleDestroyComplete(FName SessionName, bool bWasSuccessful)
{
	if (const IOnlineSessionPtr Sessions = GetSessions())
	{
		Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
	}

	// If this destroy was the first half of a re-host (CreateRoom found a stale
	// session), proceed to create now that the old one is gone.
	if (bCreateAfterDestroy)
	{
		bCreateAfterDestroy = false;
		DoCreateSession();
		return;
	}

	OnDestroyRoomComplete.Broadcast(bWasSuccessful);
}

void UPBSessionSubsystem::HandleInviteAccepted(const bool bWasSuccessful, const int32 ControllerId,
	FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult)
{
	if (!bWasSuccessful || !InviteResult.IsValid())
	{
		return;
	}
	const IOnlineSessionPtr Sessions = GetSessions();
	if (!Sessions.IsValid() || !UserId.IsValid())
	{
		return;
	}

	// Accepting a Steam invite / "Join Game" → join that session, then travel.
	if (JoinHandle.IsValid())
	{
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
	}
	JoinHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &UPBSessionSubsystem::HandleJoinComplete));
	if (!Sessions->JoinSession(*UserId, NAME_GameSession, InviteResult))
	{
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
		OnJoinRoomComplete.Broadcast(false);
	}
}

void UPBSessionSubsystem::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver,
	ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	// CRITICAL: this is the GLOBAL engine failure delegate, so it fires on the host
	// too — the host's NetDriver broadcasts ConnectionTimeout for each dropped client
	// connection. Only a real CLIENT losing the host should bounce to the menu; the
	// host must ignore its per-client drops (mirrors the engine's own host-vs-client
	// guard in UEngine::HandleNetworkFailure). Without this, one client timeout under
	// the p.NetEmulation test would tear the whole match down.
	if (!NetDriver || NetDriver->GetNetMode() != NM_Client)
	{
		return;
	}

	// The listen-server host went away (or the connection dropped). Tear down our
	// session record and return to the menu, surfacing the reason (T3.4 host-left).
	if (const IOnlineSessionPtr Sessions = GetSessions())
	{
		if (Sessions->GetNamedSession(NAME_GameSession))
		{
			Sessions->DestroySession(NAME_GameSession);
		}
	}

	OnHostLeft.Broadcast(true);

	if (APlayerController* PC = GetGameInstance() ? GetGameInstance()->GetFirstLocalPlayerController() : nullptr)
	{
		PC->ClientTravel(MainMenuMapURL, TRAVEL_Absolute);
	}
}

int32 UPBSessionSubsystem::GetNumFoundRooms() const
{
	return SearchSettings.IsValid() ? SearchSettings->SearchResults.Num() : 0;
}

FString UPBSessionSubsystem::GetRoomName(int32 SearchResultIndex) const
{
	if (SearchSettings.IsValid() && SearchSettings->SearchResults.IsValidIndex(SearchResultIndex))
	{
		FString Name;
		if (SearchSettings->SearchResults[SearchResultIndex].Session.SessionSettings.Get(PBRoomNameKey, Name))
		{
			return Name;
		}
	}
	return FString();
}

int32 UPBSessionSubsystem::GetRoomOpenSlots(int32 SearchResultIndex) const
{
	if (SearchSettings.IsValid() && SearchSettings->SearchResults.IsValidIndex(SearchResultIndex))
	{
		return SearchSettings->SearchResults[SearchResultIndex].Session.NumOpenPublicConnections;
	}
	return 0;
}
