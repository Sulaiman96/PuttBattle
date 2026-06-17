// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Match/PBMatchTypes.h"
#include "PBSessionSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPBSessionOpResult, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPBFindRoomsResult, int32, NumRooms);

/**
 * The Steam session layer (plans/03 T3.4). A GameInstance subsystem (survives map
 * travel, unlike a world-scoped one — LEARNING.md) wrapping the Online Subsystem
 * session interface: create / find / join / destroy a room, plus the Steam
 * friend-invite / "Join Game" path. The host advertises a presence lobby, travels
 * to L_Lobby as a listen server, then seamless-travels into the match; clients
 * resolve the connect string and ClientTravel in.
 *
 * UI (UMG, editor pass) drives this through the BlueprintCallable methods and the
 * result delegates. It also carries the host's chosen FPBMatchSettings across the
 * lobby→match map hop (the subsystem outlives the world; the GameState does not).
 *
 * The exact API here is UE 5.7-verified: the interface exposes Add*Delegate_Handle
 * (not AddUObject), and lobby search uses SEARCH_LOBBIES (SEARCH_PRESENCE is gone).
 */
UCLASS()
class PUTTBATTLE_API UPBSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Host a room: advertise a Steam presence lobby and travel to the lobby map. */
	UFUNCTION(BlueprintCallable, Category = "PB|Net")
	void CreateRoom(int32 MaxPlayers, const FString& RoomName);

	/** Search for joinable rooms; results land in GetRoomName/GetRoomPlayerCount. */
	UFUNCTION(BlueprintCallable, Category = "PB|Net")
	void FindRooms(int32 MaxResults = 50);

	/** Join a room from the last FindRooms results (then ClientTravel on success). */
	UFUNCTION(BlueprintCallable, Category = "PB|Net")
	void JoinRoomByIndex(int32 SearchResultIndex);

	/** Leave / tear down the current session (host or client). */
	UFUNCTION(BlueprintCallable, Category = "PB|Net")
	void DestroyRoom();

	// --- Search-result reads for the browse UI ------------------------------

	UFUNCTION(BlueprintPure, Category = "PB|Net")
	int32 GetNumFoundRooms() const;

	UFUNCTION(BlueprintPure, Category = "PB|Net")
	FString GetRoomName(int32 SearchResultIndex) const;

	UFUNCTION(BlueprintPure, Category = "PB|Net")
	int32 GetRoomOpenSlots(int32 SearchResultIndex) const;

	// --- Match settings carried lobby → match -------------------------------

	UFUNCTION(BlueprintCallable, Category = "PB|Net")
	void SetPendingMatchSettings(const FPBMatchSettings& InSettings) { PendingMatchSettings = InSettings; }

	const FPBMatchSettings& GetPendingMatchSettings() const { return PendingMatchSettings; }

	/** Map a created/joined room lives on (host listen-travels here). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "PB|Net")
	FString LobbyMapURL = TEXT("/Game/Maps/Core/L_Lobby");

	/** GameMode forced on the lobby map via the travel URL's ?game= option, so the
	 *  lobby runs APBLobbyGameMode without depending on the map's World Settings
	 *  (which keeps the offline feel-test maps on their own referee). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "PB|Net")
	FString LobbyGameModeURL = TEXT("/Script/PuttBattle.PBLobbyGameMode");

	/**
	 * Where clients are sent when the listen-server host disconnects (host-left).
	 * PLACEHOLDER: points at the lobby map until the real front-end (the deferred
	 * L_Bootstrap, see DefaultEngine.ini) ships. Loaded standalone (no ?game=), so
	 * it runs that map's own World Settings GameMode — fine as a graybox seam.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "PB|Net")
	FString MainMenuMapURL = TEXT("/Game/Maps/Core/L_Lobby");

	// --- UI-facing results --------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "PB|Net")
	FPBSessionOpResult OnCreateRoomComplete;

	UPROPERTY(BlueprintAssignable, Category = "PB|Net")
	FPBFindRoomsResult OnFindRoomsComplete;

	UPROPERTY(BlueprintAssignable, Category = "PB|Net")
	FPBSessionOpResult OnJoinRoomComplete;

	UPROPERTY(BlueprintAssignable, Category = "PB|Net")
	FPBSessionOpResult OnDestroyRoomComplete;

	/** The host quit and we were kicked back to the menu (UI shows the reason). */
	UPROPERTY(BlueprintAssignable, Category = "PB|Net")
	FPBSessionOpResult OnHostLeft;

private:
	IOnlineSessionPtr GetSessions() const;
	FUniqueNetIdPtr GetLocalUserId() const;

	/** Build settings + fire CreateSession (the body of CreateRoom; also re-entered
	 *  after an existing session finishes destroying — DestroySession is async). */
	void DoCreateSession();

	// OSS completion handlers (signatures are 5.7-exact).
	void HandleCreateComplete(FName SessionName, bool bWasSuccessful);
	void HandleFindComplete(bool bWasSuccessful);
	void HandleJoinComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void HandleDestroyComplete(FName SessionName, bool bWasSuccessful);
	void HandleInviteAccepted(const bool bWasSuccessful, const int32 ControllerId,
		FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult);

	/** Travel the local client into a joined session (resolved connect string). */
	void TravelToResolvedSession();

	/** Network failure (client lost the host) → return to the menu (host-left). */
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);

	/** Kept alive between FindRooms and its completion (the interface holds a ref). */
	TSharedPtr<FOnlineSessionSearch> SearchSettings;

	/** Host's chosen rules, surfaced to APBMatchGameMode after the travel. */
	FPBMatchSettings PendingMatchSettings;

	/** Display name advertised with the session (for the browse list). */
	FString PendingRoomName;

	/** Max players for the pending CreateRoom (held across the destroy→create chain). */
	int32 PendingMaxPlayers = 6;

	/** True while a CreateRoom is waiting on a prior session to finish destroying. */
	bool bCreateAfterDestroy = false;

	// Delegate handles (the delegates themselves are built inline at call sites).
	FDelegateHandle CreateHandle;
	FDelegateHandle FindHandle;
	FDelegateHandle JoinHandle;
	FDelegateHandle DestroyHandle;
	FDelegateHandle InviteHandle;
	FDelegateHandle NetworkFailureHandle;
};
