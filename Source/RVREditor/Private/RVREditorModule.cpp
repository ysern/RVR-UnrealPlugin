// Copyright 2026, Iurii Sernivka.

#include "RVREditor.h"

#include "RVR.h"
#include "RVRSettings.h"
#include "SRVRStatusDialog.h"

#include "Editor.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/App.h"
#include "PlayInEditorDataTypes.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "ToolMenus.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"

#define LOCTEXT_NAMESPACE "FRVREditorModule"

DEFINE_LOG_CATEGORY_STATIC(LogRVREditor, Log, All);

namespace
{
FLinearColor BubbleColour(ERVRBubble Bubble)
{
	switch (Bubble)
	{
	case ERVRBubble::Green: return FLinearColor(0.10f, 0.80f, 0.25f);
	case ERVRBubble::Yellow: return FLinearColor(0.95f, 0.75f, 0.05f);
	case ERVRBubble::Red:
	default: return FLinearColor(0.90f, 0.12f, 0.10f);
	}
}

/** The project's name and its open level: how the Host Session describes itself. */
void OpenHostSessionForThisProject()
{
	FString Level;
	if (GEditor)
	{
		if (UWorld* World = GEditor->GetEditorWorldContext().World())
		{
			Level = World->GetMapName();
		}
	}
	FRVRModule::Get().OpenHostSession(TEXT("Unreal Editor"), FApp::GetProjectName(), Level);
}
} // namespace

void FRVREditorModule::StartupModule()
{
	// The logo and the bubble. The logo is the product's own, copied from its documentation assets.
	const FString Resources = IPluginManager::Get().FindPlugin(TEXT("RVR"))->GetBaseDir() / TEXT("Resources");
	Style = MakeShared<FSlateStyleSet>(TEXT("RVRStyle"));
	Style->Set("RVR.Logo", new FSlateImageBrush(Resources / TEXT("logo_64p.png"), FVector2D(46.0f, 20.0f)));
	Style->Set("RVR.Bubble", new FSlateRoundedBoxBrush(FLinearColor::White, 4.0f, FVector2D(8.0f, 8.0f)));
	FSlateStyleRegistry::RegisterSlateStyle(*Style);

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FRVREditorModule::RegisterToolbar));
	StatusHandle = FRVRModule::Get().OnStatusChanged.AddRaw(this, &FRVREditorModule::OnStatusChanged);

	const URVRSettings* Settings = GetDefault<URVRSettings>();
	if (Settings->bGoOnlineAtStartup)
	{
		UE_LOG(LogRVREditor, Log, TEXT("Going online, as this project's settings ask."));
		FRVRModule::Get().GoOnline(Settings->SignallingServer, Settings->ConnectionProfile.FilePath);
		FRVRModule::Get().ChooseLobby(Settings->Lobby);
		OpenHostSessionForThisProject();
	}
}

void FRVREditorModule::ShutdownModule()
{
	if (FRVRModule::IsAvailable())
	{
		FRVRModule::Get().OnStatusChanged.Remove(StatusHandle);
	}
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
	if (Style.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*Style);
		Style.Reset();
	}
}

void FRVREditorModule::RegisterToolbar()
{
	FToolMenuOwnerScoped Owner(this);
	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.User");
	FToolMenuSection& Section = Menu->FindOrAddSection("RVR");
	const FSlateStyleSet* S = Style.Get();

	TSharedRef<SWidget> Button =
		SNew(SButton)
		.ButtonStyle(FAppStyle::Get(), "SimpleButton")
		.ContentPadding(FMargin(4.0f, 2.0f))
		.ToolTipText_Lambda([]() { return PresentRVRStatus(FRVRModule::Get().GetStatus()).Tooltip; })
		.OnClicked_Lambda([this]()
		{
			OpenStatusDialog();
			return FReply::Handled();
		})
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			.Padding(FMargin(0.0f, 2.0f, 6.0f, 0.0f))
			[
				SNew(SImage).Image(S->GetBrush("RVR.Logo"))
			]
			+ SOverlay::Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Top)
			[
				SNew(SImage)
				.Image(S->GetBrush("RVR.Bubble"))
				.ColorAndOpacity_Lambda([]() { return FSlateColor(BubbleColour(PresentRVRStatus(FRVRModule::Get().GetStatus()).Bubble)); })
			]
		];

	Section.AddEntry(FToolMenuEntry::InitWidget("RVRStatus", Button, LOCTEXT("RVRLabel", "RVR")));
}

void FRVREditorModule::OpenStatusDialog()
{
	if (TSharedPtr<SWindow> Existing = Dialog.Pin())
	{
		Existing->BringToFront();
		return;
	}
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(LOCTEXT("DialogTitle", "RVR"))
		.ClientSize(FVector2D(460.0f, 520.0f))
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		[
			SNew(SRVRStatusDialog)
		];
	Dialog = Window;
	FSlateApplication::Get().AddWindow(Window);
}

void FRVREditorModule::OnStatusChanged()
{
	AnswerStartRequest();
	FollowShouldRunVr();
}

void FRVREditorModule::AnswerStartRequest()
{
	const FRVRStatus& S = FRVRModule::Get().GetStatus();
	// rvr-host-api clears the request once it is answered, so each is seen here once.
	if (!S.bStartRequestPending)
	{
		return;
	}
	// What only Unreal can know: whether the Editor can start VR Preview now. Starting it waits for the
	// Network Session (rvr-host-api's should_run_vr); the answer is given at once, inside the reservation.
	const TSharedPtr<IPlugin> OpenXR = IPluginManager::Get().FindPlugin(TEXT("OpenXR"));
	const bool bOpenXR = OpenXR.IsValid() && OpenXR->IsEnabled();
	const bool bIdle = GEditor && GEditor->PlayWorld == nullptr && !GEditor->IsPlaySessionInProgress();
	const bool bAccept = bOpenXR && bIdle;
	UE_LOG(LogRVREditor, Log, TEXT("A Reviewer (%s) asks to start: %s."), *S.StartRequestDevice,
		bAccept ? TEXT("accepted") : (bOpenXR ? TEXT("refused, a play session is running") : TEXT("refused, the OpenXR plugin is not enabled")));
	FRVRModule::Get().AnswerStart(S.StartRequestId, bAccept, bAccept ? FString() : TEXT("host_not_ready"));
}

void FRVREditorModule::FollowShouldRunVr()
{
	const bool bShould = FRVRModule::Get().GetStatus().bShouldRunVr;
	if (bShould == bInVr || !GEditor)
	{
		return;
	}
	bInVr = bShould;
	if (bShould)
	{
		UE_LOG(LogRVREditor, Log, TEXT("The Device is connected: starting VR Preview."));
		FRequestPlaySessionParams Params;
		Params.SessionPreviewTypeOverride = EPlaySessionPreviewType::VRPreview;
		GEditor->RequestPlaySession(Params);
	}
	else
	{
		UE_LOG(LogRVREditor, Log, TEXT("The Review Session is over: stopping VR Preview."));
		GEditor->RequestEndPlayMap();
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRVREditorModule, RVREditor)
