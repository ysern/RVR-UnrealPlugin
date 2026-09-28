// Copyright 2026, Iurii Sernivka.

#include "SRVRStatusDialog.h"

#include "RVR.h"
#include "RVRSettings.h"

#include "Editor.h"
#include "Engine/World.h"
#include "ISettingsModule.h"
#include "Misc/App.h"
#include "Modules/ModuleManager.h"
#include "Styling/AppStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SRVRStatusDialog"

namespace
{
const FRVRStatus& Now()
{
	return FRVRModule::Get().GetStatus();
}

const TCHAR* SignallingWord(ERVRSignalling S)
{
	switch (S)
	{
	case ERVRSignalling::Connecting: return TEXT("connecting");
	case ERVRSignalling::Enrolling: return TEXT("enrolling");
	case ERVRSignalling::Online: return TEXT("online");
	case ERVRSignalling::Present: return TEXT("present");
	case ERVRSignalling::Refused: return TEXT("refused");
	case ERVRSignalling::Offline:
	default: return TEXT("offline");
	}
}

TSharedRef<SWidget> Row(const FText& Label, TAttribute<FText> Value)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(0, 2, 12, 2)
		[
			SNew(SBox).WidthOverride(120.0f)[SNew(STextBlock).Text(Label).ColorAndOpacity(FSlateColor::UseSubduedForeground())]
		]
		+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0, 2)
		[
			SNew(STextBlock).Text(Value).AutoWrapText(true)
		];
}

FString CurrentLevel()
{
	if (GEditor)
	{
		if (UWorld* World = GEditor->GetEditorWorldContext().World())
		{
			return World->GetMapName();
		}
	}
	return FString();
}
} // namespace

void SRVRStatusDialog::Construct(const FArguments& InArgs)
{
	StatusHandle = FRVRModule::Get().OnStatusChanged.AddSP(this, &SRVRStatusDialog::OnStatusChanged);
	OnStatusChanged();

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(12.0f)
		[
			SNew(SVerticalBox)
			// The cog: Project Settings, at RVR.
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				.ToolTipText(LOCTEXT("CogTip", "Open RVR's Project Settings"))
				.OnClicked(this, &SRVRStatusDialog::OnCog)
				[
					SNew(SImage).Image(FAppStyle::GetBrush("Icons.Settings")).ColorAndOpacity(FSlateColor::UseForeground())
				]
			]
			+ SVerticalBox::Slot().AutoHeight()[Row(LOCTEXT("Service", "Service"), MakeAttributeSP(this, &SRVRStatusDialog::ServiceLine))]
			+ SVerticalBox::Slot().AutoHeight()[Row(LOCTEXT("Signalling", "Signalling"), MakeAttributeSP(this, &SRVRStatusDialog::SignallingLine))]
			+ SVerticalBox::Slot().AutoHeight()[Row(LOCTEXT("Server", "Server"), MakeAttributeSP(this, &SRVRStatusDialog::ServerLine))]
			+ SVerticalBox::Slot().AutoHeight()[Row(LOCTEXT("Enrolment", "Enrolment"), MakeAttributeSP(this, &SRVRStatusDialog::EnrolmentLine))]
			+ SVerticalBox::Slot().AutoHeight()[Row(LOCTEXT("Lobby", "Lobby"), MakeAttributeSP(this, &SRVRStatusDialog::LobbyLine))]
			+ SVerticalBox::Slot().AutoHeight()[Row(LOCTEXT("HostSession", "Host Session"), MakeAttributeSP(this, &SRVRStatusDialog::HostSessionLine))]
			+ SVerticalBox::Slot().AutoHeight()[Row(LOCTEXT("ReviewSession", "Review Session"), MakeAttributeSP(this, &SRVRStatusDialog::ReviewLine))]
			+ SVerticalBox::Slot().AutoHeight()[Row(LOCTEXT("NetworkSession", "Network Session"), MakeAttributeSP(this, &SRVRStatusDialog::NetworkLine))]
			+ SVerticalBox::Slot().AutoHeight()[Row(LOCTEXT("LastReason", "Last reason"), MakeAttributeSP(this, &SRVRStatusDialog::ReasonLine))]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 10)[SNew(SSeparator)]
			// The Lobby, by name.
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
			[
				SAssignNew(LobbyCombo, SComboBox<TSharedPtr<FString>>)
				.OptionsSource(&LobbyNames)
				.OnGenerateWidget_Lambda([](TSharedPtr<FString> Name) { return SNew(STextBlock).Text(FText::FromString(*Name)); })
				.OnSelectionChanged_Lambda([](TSharedPtr<FString> Name, ESelectInfo::Type How)
				{
					if (!Name.IsValid() || How == ESelectInfo::Direct)
					{
						return;
					}
					URVRSettings* Settings = GetMutableDefault<URVRSettings>();
					Settings->Lobby = *Name;
					Settings->TryUpdateDefaultConfigFile();
					FRVRModule::Get().ChooseLobby(*Name);
				})
				[
					SNew(STextBlock).Text_Lambda([]()
					{
						const FString Chosen = GetDefault<URVRSettings>()->Lobby;
						return Chosen.IsEmpty() ? LOCTEXT("ChooseLobby", "Choose a Lobby") : FText::FromString(Chosen);
					})
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)
				[
					SNew(SButton)
					.IsEnabled_Lambda([]() { return Now().bInstalled; })
					.Text_Lambda([]()
					{
						const ERVRSignalling S = Now().Signalling;
						return S == ERVRSignalling::Offline || S == ERVRSignalling::Refused ? LOCTEXT("GoOnline", "Go online") : LOCTEXT("GoOffline", "Go offline");
					})
					.OnClicked(this, &SRVRStatusDialog::OnOnline)
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton)
					.IsEnabled_Lambda([]() { return Now().bServiceAvailable && (!Now().bHostSessionOpen || Now().bHostSessionYours); })
					.Text_Lambda([]() { return Now().bHostSessionYours ? LOCTEXT("CloseHost", "Close Host Session") : LOCTEXT("OpenHost", "Open Host Session"); })
					.OnClicked(this, &SRVRStatusDialog::OnHostSession)
				]
			]
		]
	];
}

SRVRStatusDialog::~SRVRStatusDialog()
{
	if (FRVRModule::IsAvailable())
	{
		FRVRModule::Get().OnStatusChanged.Remove(StatusHandle);
	}
}

void SRVRStatusDialog::OnStatusChanged()
{
	LobbyNames.Reset();
	for (const FRVRLobby& L : Now().Lobbies)
	{
		LobbyNames.Add(MakeShared<FString>(L.Name));
	}
	if (LobbyCombo.IsValid())
	{
		LobbyCombo->RefreshOptions();
	}
}

FText SRVRStatusDialog::ServiceLine() const
{
	const FRVRStatus& S = Now();
	if (!S.bInstalled)
	{
		return FText::FromString(S.InstallMessage);
	}
	return S.bServiceAvailable ? LOCTEXT("Running", "running") : LOCTEXT("NotRunning", "not running");
}

FText SRVRStatusDialog::SignallingLine() const
{
	const FRVRStatus& S = Now();
	if (S.Signalling == ERVRSignalling::Connecting && !S.Error.IsEmpty())
	{
		return FText::Format(LOCTEXT("ConnectingError", "connecting -- {0}; next try in {1} s"), FText::FromString(S.Error),
			FText::AsNumber(FMath::DivideAndRoundUp(S.RetryInMs, 1000)));
	}
	if (S.Signalling == ERVRSignalling::Refused)
	{
		return FText::Format(LOCTEXT("RefusedLine", "refused -- {0}"), FText::FromString(S.Error));
	}
	return FText::FromString(SignallingWord(S.Signalling));
}

FText SRVRStatusDialog::ServerLine() const
{
	const FRVRStatus& S = Now();
	return S.Server.IsEmpty() ? LOCTEXT("NoServer", "none yet")
							  : FText::Format(LOCTEXT("ServerFrom", "{0}, from {1}"), FText::FromString(S.Server), FText::FromString(S.ServerSource));
}

FText SRVRStatusDialog::EnrolmentLine() const
{
	const FRVRStatus& S = Now();
	if (S.Signalling != ERVRSignalling::Enrolling || S.UserCode.IsEmpty())
	{
		return LOCTEXT("NoCode", "--");
	}
	return FText::Format(LOCTEXT("Code", "approve the code {0} at {1}"), FText::FromString(S.UserCode), FText::FromString(S.VerificationUri));
}

FText SRVRStatusDialog::LobbyLine() const
{
	const FString Name = Now().LobbyName();
	return Name.IsEmpty() ? LOCTEXT("NoLobby", "not present in a Lobby") : FText::FromString(Name);
}

FText SRVRStatusDialog::HostSessionLine() const
{
	const FRVRStatus& S = Now();
	if (!S.bHostSessionOpen)
	{
		return LOCTEXT("HostClosed", "closed");
	}
	return S.bHostSessionYours
		? FText::Format(LOCTEXT("HostYours", "open: {0} {1} {2}"), FText::FromString(S.Application), FText::FromString(S.Project), FText::FromString(S.Scene))
		: LOCTEXT("HostOther", "open, held by another application");
}

FText SRVRStatusDialog::ReviewLine() const
{
	const FRVRStatus& S = Now();
	switch (S.Review)
	{
	case ERVRReview::Requested: return FText::Format(LOCTEXT("Requested", "requested by {0}"), FText::FromString(S.ReviewDevice));
	case ERVRReview::Accepted: return FText::Format(LOCTEXT("Accepted", "in progress with {0}"), FText::FromString(S.ReviewDevice));
	default: return LOCTEXT("NoReview", "none");
	}
}

FText SRVRStatusDialog::NetworkLine() const
{
	switch (Now().Network)
	{
	case ERVRNetwork::Idle: return LOCTEXT("Idle", "idle");
	case ERVRNetwork::Streaming: return LOCTEXT("StreamingLine", "streaming");
	default: return LOCTEXT("NoNetwork", "none");
	}
}

FText SRVRStatusDialog::ReasonLine() const
{
	const FString& R = Now().LastReason;
	return R.IsEmpty() ? LOCTEXT("NoReason", "--") : FText::FromString(R);
}

FReply SRVRStatusDialog::OnOnline()
{
	const ERVRSignalling S = Now().Signalling;
	if (S == ERVRSignalling::Offline || S == ERVRSignalling::Refused)
	{
		const URVRSettings* Settings = GetDefault<URVRSettings>();
		FRVRModule::Get().GoOnline(Settings->SignallingServer, Settings->ConnectionProfile.FilePath);
		FRVRModule::Get().ChooseLobby(Settings->Lobby);
	}
	else
	{
		FRVRModule::Get().GoOffline();
	}
	return FReply::Handled();
}

FReply SRVRStatusDialog::OnHostSession()
{
	if (Now().bHostSessionYours)
	{
		FRVRModule::Get().CloseHostSession();
		return FReply::Handled();
	}
	FRVRModule::Get().ChooseLobby(GetDefault<URVRSettings>()->Lobby);
	FRVRModule::Get().OpenHostSession(TEXT("Unreal Editor"), FApp::GetProjectName(), CurrentLevel());
	return FReply::Handled();
}

FReply SRVRStatusDialog::OnCog()
{
	FModuleManager::LoadModuleChecked<ISettingsModule>("Settings").ShowViewer("Project", "Plugins", "RVR");
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
