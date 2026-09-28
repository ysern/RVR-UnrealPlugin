// Copyright 2026, Iurii Sernivka.

#include "RVR.h"

#include "Async/Async.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include "rvr/host_api/host_api.hpp"
#include "Windows/HideWindowsPlatformTypes.h"
#endif

#define LOCTEXT_NAMESPACE "FRVRModule"

DEFINE_LOG_CATEGORY_STATIC(LogRVR, Log, All);

/** rvr-host-api's C++ wrapper, kept out of the public header so no includer sees windows.h. */
class FRVRClient
{
public:
#if PLATFORM_WINDOWS
	rvr::host_api::Client Api;
#endif
};

namespace
{
using FAliveToken = TWeakPtr<bool, ESPMode::ThreadSafe>;

FString Utf8(const std::string& S)
{
	return FString(UTF8_TO_TCHAR(S.c_str()));
}

std::string Std(const FString& S)
{
	return std::string(TCHAR_TO_UTF8(*S));
}

const TCHAR* SourceName(int Source)
{
	switch (Source)
	{
	case 1: return TEXT("this project's settings");
	case 2: return TEXT("the RVR_SIGNALLING_SERVER environment variable");
	case 3: return TEXT("RVR's configuration file");
	case 4: return TEXT("RVR's built-in default");
	default: return TEXT("nowhere");
	}
}
} // namespace

FString FRVRStatus::LobbyName() const
{
	for (const FRVRLobby& Lobby : Lobbies)
	{
		if (Lobby.Id == LobbyId)
		{
			return Lobby.Name;
		}
	}
	return FString();
}

FRVRModule& FRVRModule::Get()
{
	return FModuleManager::LoadModuleChecked<FRVRModule>("RVR");
}

bool FRVRModule::IsAvailable()
{
	return FModuleManager::Get().IsModuleLoaded("RVR");
}

void FRVRModule::StartupModule()
{
	Alive = MakeShared<bool, ESPMode::ThreadSafe>(true);
	Client = MakeUnique<FRVRClient>();
#if PLATFORM_WINDOWS
	Status.bInstalled = Client->Api.Available();
	Status.InstallMessage = Utf8(Client->Api.LoadMessage());
	UE_LOG(LogRVR, Log, TEXT("%s"), *Status.InstallMessage);
	if (Status.bInstalled)
	{
		// The library's thread only notes the change; the status is read on the game thread.
		FAliveToken* Token = new FAliveToken(Alive);
		ChangeToken = Token;
		Client->Api.SetOnChange(
			[](void* User)
			{
				FAliveToken Weak = *static_cast<FAliveToken*>(User);
				AsyncTask(ENamedThreads::GameThread,
					[Weak]()
					{
						if (Weak.IsValid() && FRVRModule::IsAvailable())
						{
							FRVRModule::Get().Refresh();
						}
					});
			},
			Token);
	}
#else
	Status.InstallMessage = TEXT("RVR is not available on this platform.");
#endif
	Refresh();
}

void FRVRModule::ShutdownModule()
{
#if PLATFORM_WINDOWS
	if (Client && Status.bInstalled)
	{
		Client->Api.SetOnChange(nullptr, nullptr);
	}
#endif
	Alive.Reset();
	Client.Reset();
	delete static_cast<FAliveToken*>(ChangeToken);
	ChangeToken = nullptr;
}

void FRVRModule::Refresh()
{
#if PLATFORM_WINDOWS
	if (Client && Status.bInstalled)
	{
		const rvr::host_api::Status S = Client->Api.GetStatus();
		FRVRStatus Next;
		Next.bInstalled = true;
		Next.InstallMessage = Status.InstallMessage;
		Next.bServiceAvailable = S.service_available;
		Next.Signalling = static_cast<ERVRSignalling>(S.signalling);
		Next.Error = Utf8(S.error);
		Next.RetryInMs = S.retry_in_ms;
		Next.Server = Utf8(S.server);
		Next.ServerSource = SourceName(S.source);
		Next.UserCode = Utf8(S.user_code);
		Next.VerificationUri = Utf8(S.verification_uri);
		for (const rvr::host_api::Lobby& L : S.lobbies)
		{
			Next.Lobbies.Add({Utf8(L.id), Utf8(L.name)});
		}
		Next.LobbyId = Utf8(S.lobby);
		Next.bHostSessionOpen = S.host_session_open;
		Next.bHostSessionYours = S.host_session_yours;
		Next.Application = Utf8(S.application);
		Next.Project = Utf8(S.project);
		Next.Scene = Utf8(S.scene);
		Next.Review = static_cast<ERVRReview>(S.review);
		Next.ReviewId = Utf8(S.review_id);
		Next.ReviewDevice = Utf8(S.review_device);
		Next.bStartRequestPending = S.start_request_pending;
		Next.StartRequestId = Utf8(S.start_request_id);
		Next.StartRequestDevice = Utf8(S.start_request_device);
		Next.Network = static_cast<ERVRNetwork>(S.network);
		Next.LastReason = Utf8(S.last_reason);
		Next.bShouldRunVr = Client->Api.ShouldRunVr();
		Status = MoveTemp(Next);
	}
#endif
	OnStatusChanged.Broadcast();
}

void FRVRModule::GoOnline(const FString& Server, const FString& ConnectionProfile)
{
#if PLATFORM_WINDOWS
	if (Status.bInstalled)
	{
		Client->Api.GoOnline(Std(Server), Std(ConnectionProfile));
	}
#endif
}

void FRVRModule::GoOffline()
{
#if PLATFORM_WINDOWS
	if (Status.bInstalled)
	{
		Client->Api.GoOffline();
	}
#endif
}

void FRVRModule::ChooseLobby(const FString& Lobby)
{
#if PLATFORM_WINDOWS
	if (Status.bInstalled && !Lobby.IsEmpty())
	{
		Client->Api.ChooseLobby(Std(Lobby));
	}
#endif
}

void FRVRModule::OpenHostSession(const FString& Application, const FString& Project, const FString& Scene)
{
#if PLATFORM_WINDOWS
	if (Status.bInstalled)
	{
		Client->Api.OpenHostSession(Std(Application), Std(Project), Std(Scene));
	}
#endif
}

void FRVRModule::CloseHostSession()
{
#if PLATFORM_WINDOWS
	if (Status.bInstalled)
	{
		Client->Api.CloseHostSession();
	}
#endif
}

void FRVRModule::AnswerStart(const FString& ReviewId, bool bAccept, const FString& Reason)
{
#if PLATFORM_WINDOWS
	if (Status.bInstalled)
	{
		Client->Api.AnswerStart(Std(ReviewId), bAccept, Std(Reason));
	}
#endif
}

void FRVRModule::EndReview()
{
#if PLATFORM_WINDOWS
	if (Status.bInstalled)
	{
		Client->Api.EndReview();
	}
#endif
}

// ---- The toolbar's presentation ----

FRVRPresentation PresentRVRStatus(const FRVRStatus& S)
{
	FRVRPresentation P;
	if (!S.bInstalled)
	{
		P.Bubble = ERVRBubble::Red;
		P.Tooltip = FText::FromString(S.InstallMessage.IsEmpty() ? TEXT("RVR is not installed.") : S.InstallMessage);
		return P;
	}
	if (!S.bServiceAvailable)
	{
		P.Bubble = ERVRBubble::Red;
		P.Tooltip = LOCTEXT("ServiceNotRunning", "RVR: the RVR Service is not running.");
		return P;
	}
	if (S.Network != ERVRNetwork::None)
	{
		P.Bubble = ERVRBubble::Green;
		const FText Device = S.ReviewDevice.IsEmpty() ? LOCTEXT("ADevice", "a Device") : FText::FromString(S.ReviewDevice);
		P.Tooltip = S.Network == ERVRNetwork::Streaming
			? FText::Format(LOCTEXT("Streaming", "RVR: connected to {0}, streaming."), Device)
			: FText::Format(LOCTEXT("Connected", "RVR: connected to {0}."), Device);
		return P;
	}
	switch (S.Signalling)
	{
	case ERVRSignalling::Present:
		P.Bubble = ERVRBubble::Yellow;
		P.Tooltip = S.Review == ERVRReview::None
			? FText::Format(LOCTEXT("Present", "RVR: online, in the Lobby {0}, waiting for a Reviewer."), FText::FromString(S.LobbyName()))
			: FText::Format(LOCTEXT("Reviewer", "RVR: a Reviewer ({0}) is on the way."), FText::FromString(S.ReviewDevice));
		return P;
	case ERVRSignalling::Online:
		P.Bubble = ERVRBubble::Yellow;
		P.Tooltip = LOCTEXT("Online", "RVR: online, not in a Lobby. Choose one and open the Host Session.");
		return P;
	case ERVRSignalling::Enrolling:
		P.Bubble = ERVRBubble::Red;
		P.Tooltip = S.UserCode.IsEmpty()
			? LOCTEXT("EnrollingNoCode", "RVR: not connected -- enrolling this machine.")
			: FText::Format(LOCTEXT("Enrolling", "RVR: not connected -- the code {0} is waiting for approval at {1}."),
				FText::FromString(S.UserCode), FText::FromString(S.VerificationUri));
		return P;
	case ERVRSignalling::Connecting:
		P.Bubble = ERVRBubble::Red;
		P.Tooltip = S.Error.IsEmpty()
			? FText::Format(LOCTEXT("Connecting", "RVR: not connected -- connecting to {0}."), FText::FromString(S.Server))
			: FText::Format(LOCTEXT("Retrying", "RVR: not connected -- {0}; trying {1} again in {2} s."),
				FText::FromString(S.Error), FText::FromString(S.Server), FText::AsNumber(FMath::DivideAndRoundUp(S.RetryInMs, 1000)));
		return P;
	case ERVRSignalling::Refused:
		P.Bubble = ERVRBubble::Red;
		P.Tooltip = FText::Format(LOCTEXT("Refused", "RVR: not connected -- {0}."), FText::FromString(S.Error));
		return P;
	case ERVRSignalling::Offline:
	default:
		P.Bubble = ERVRBubble::Red;
		P.Tooltip = LOCTEXT("Offline", "RVR: offline.");
		return P;
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRVRModule, RVR)
