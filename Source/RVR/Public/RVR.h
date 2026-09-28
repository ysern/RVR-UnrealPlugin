// Copyright 2026, Iurii Sernivka.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FRVRClient;

enum class ERVRSignalling : uint8
{
	Offline,
	Connecting,
	Enrolling,
	Online,
	Present,
	Refused,
};

enum class ERVRReview : uint8
{
	None,
	Requested,
	Accepted,
};

enum class ERVRNetwork : uint8
{
	None,
	Idle,
	Streaming,
};

struct FRVRLobby
{
	FString Id;
	FString Name;
};

/**
 * What rvr-host-api reports, in Unreal's types. A copy, made on the game thread
 * whenever the library says the status changed. The plugin decides nothing from
 * it that rvr-host-api decides; it shows it, and passes the Operator's commands on.
 */
struct RVR_API FRVRStatus
{
	/** The library RVR installs was found and loaded. */
	bool bInstalled = false;
	/** "RVR is not installed." and the like. */
	FString InstallMessage;

	bool bServiceAvailable = false;
	ERVRSignalling Signalling = ERVRSignalling::Offline;
	FString Error;
	int32 RetryInMs = 0;
	FString Server;
	/** Where the address came from, in words. */
	FString ServerSource;
	FString UserCode;
	FString VerificationUri;

	TArray<FRVRLobby> Lobbies;
	FString LobbyId;

	bool bHostSessionOpen = false;
	bool bHostSessionYours = false;
	FString Application;
	FString Project;
	FString Scene;

	ERVRReview Review = ERVRReview::None;
	FString ReviewId;
	FString ReviewDevice;
	bool bStartRequestPending = false;
	FString StartRequestId;
	FString StartRequestDevice;

	ERVRNetwork Network = ERVRNetwork::None;
	FString LastReason;

	/** rvr-host-api's decision: be in VR now. */
	bool bShouldRunVr = false;

	/** The name of the Lobby present in, or empty. */
	FString LobbyName() const;
};

/** The toolbar's status bubble, in traffic-light colours. */
enum class ERVRBubble : uint8
{
	/** No connection with the Signalling Server. */
	Red,
	/** Connected to the Signalling Server. */
	Yellow,
	/** Connected to the Device: a Network Session is up. */
	Green,
};

struct FRVRPresentation
{
	ERVRBubble Bubble = ERVRBubble::Red;
	/** The state in words, including why it is red. */
	FText Tooltip;
};

/** How a status is shown on the toolbar. Pure: tested on its own. */
RVR_API FRVRPresentation PresentRVRStatus(const FRVRStatus& Status);

class RVR_API FRVRModule : public IModuleInterface
{
public:
	static FRVRModule& Get();
	static bool IsAvailable();

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** The latest status. Game thread. */
	const FRVRStatus& GetStatus() const { return Status; }

	/** Broadcast on the game thread whenever the status changes. */
	FSimpleMulticastDelegate OnStatusChanged;

	// The Operator's commands, passed to rvr-host-api. An empty string means "use RVR's own configuration".
	void GoOnline(const FString& Server, const FString& ConnectionProfile);
	void GoOffline();
	/** A Lobby by name or identifier; rvr-host-api resolves and remembers it. */
	void ChooseLobby(const FString& Lobby);
	void OpenHostSession(const FString& Application, const FString& Project, const FString& Scene);
	void CloseHostSession();
	void AnswerStart(const FString& ReviewId, bool bAccept, const FString& Reason);
	void EndReview();

private:
	void Refresh();

	TUniquePtr<FRVRClient> Client;
	FRVRStatus Status;
	TSharedPtr<bool, ESPMode::ThreadSafe> Alive;
	/** What the library's change callback is given: a weak reference to Alive. */
	void* ChangeToken = nullptr;
};
