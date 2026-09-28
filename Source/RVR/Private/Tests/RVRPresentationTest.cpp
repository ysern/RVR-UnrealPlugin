// Copyright 2026, Iurii Sernivka.
//
// The toolbar bubble: red for no connection with the Signalling Server, yellow
// for connected to it, green for connected to the Device -- and the tooltip
// saying so in words, including why it is red.

#include "RVR.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRVRPresentationTest, "RVR.Toolbar.StatusBubble",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

namespace
{
FRVRStatus Available(ERVRSignalling Signalling)
{
	FRVRStatus S;
	S.bInstalled = true;
	S.bServiceAvailable = true;
	S.Signalling = Signalling;
	S.Server = TEXT("wss://signal.example.test:443");
	return S;
}
} // namespace

bool FRVRPresentationTest::RunTest(const FString& Parameters)
{
	auto Check = [this](const TCHAR* What, const FRVRStatus& S, ERVRBubble Bubble, const TCHAR* Words)
	{
		const FRVRPresentation P = PresentRVRStatus(S);
		TestEqual(FString::Printf(TEXT("%s: colour"), What), static_cast<int32>(P.Bubble), static_cast<int32>(Bubble));
		TestTrue(FString::Printf(TEXT("%s: tooltip \"%s\" says \"%s\""), What, *P.Tooltip.ToString(), Words), P.Tooltip.ToString().Contains(Words));
	};

	FRVRStatus NotInstalled;
	NotInstalled.InstallMessage = TEXT("RVR is not installed.");
	Check(TEXT("not installed"), NotInstalled, ERVRBubble::Red, TEXT("not installed"));

	FRVRStatus NoService;
	NoService.bInstalled = true;
	Check(TEXT("Service not running"), NoService, ERVRBubble::Red, TEXT("not running"));

	Check(TEXT("offline"), Available(ERVRSignalling::Offline), ERVRBubble::Red, TEXT("offline"));

	FRVRStatus Connecting = Available(ERVRSignalling::Connecting);
	Connecting.Error = TEXT("could not connect");
	Connecting.RetryInMs = 1500;
	Check(TEXT("connecting"), Connecting, ERVRBubble::Red, TEXT("could not connect; trying wss://signal.example.test:443 again in 2 s"));

	FRVRStatus Enrolling = Available(ERVRSignalling::Enrolling);
	Enrolling.UserCode = TEXT("XKDD-RCGR");
	Enrolling.VerificationUri = TEXT("https://signal.example.test/enrol");
	Check(TEXT("enrolling"), Enrolling, ERVRBubble::Red, TEXT("the code XKDD-RCGR is waiting for approval"));

	FRVRStatus Refused = Available(ERVRSignalling::Refused);
	Refused.Error = TEXT("no Signalling Server is configured");
	Check(TEXT("refused"), Refused, ERVRBubble::Red, TEXT("no Signalling Server is configured"));

	Check(TEXT("online"), Available(ERVRSignalling::Online), ERVRBubble::Yellow, TEXT("online, not in a Lobby"));

	FRVRStatus Present = Available(ERVRSignalling::Present);
	Present.Lobbies = {{TEXT("l_1"), TEXT("New Show")}};
	Present.LobbyId = TEXT("l_1");
	Check(TEXT("present"), Present, ERVRBubble::Yellow, TEXT("in the Lobby New Show"));

	FRVRStatus Idle = Present;
	Idle.Review = ERVRReview::Accepted;
	Idle.ReviewDevice = TEXT("Review Headset 2");
	Idle.Network = ERVRNetwork::Idle;
	Check(TEXT("Device connected"), Idle, ERVRBubble::Green, TEXT("connected to Review Headset 2"));

	FRVRStatus Streaming = Idle;
	Streaming.Network = ERVRNetwork::Streaming;
	Check(TEXT("streaming"), Streaming, ERVRBubble::Green, TEXT("streaming"));

	// The Device connected directly, with no Review Session: still green.
	FRVRStatus Direct = Available(ERVRSignalling::Offline);
	Direct.Network = ERVRNetwork::Idle;
	Check(TEXT("a Device with no Review Session"), Direct, ERVRBubble::Green, TEXT("connected to a Device"));
	return true;
}

#endif
