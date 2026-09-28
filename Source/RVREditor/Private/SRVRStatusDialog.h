// Copyright 2026, Iurii Sernivka.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/SCompoundWidget.h"

/**
 * The Status Dialog: the host side's state, laid out as the Device's status panel,
 * and the Operator's controls. Every line is read from rvr-host-api's status as it
 * is painted, so it updates while open; the Lobby list is rebuilt on each change.
 */
class SRVRStatusDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SRVRStatusDialog) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SRVRStatusDialog() override;

private:
	void OnStatusChanged();

	FText ServiceLine() const;
	FText SignallingLine() const;
	FText ServerLine() const;
	FText EnrolmentLine() const;
	FText LobbyLine() const;
	FText HostSessionLine() const;
	FText ReviewLine() const;
	FText NetworkLine() const;
	FText ReasonLine() const;

	FReply OnOnline();
	FReply OnHostSession();
	FReply OnCog();

	TArray<TSharedPtr<FString>> LobbyNames;
	TSharedPtr<SComboBox<TSharedPtr<FString>>> LobbyCombo;
	FDelegateHandle StatusHandle;
};
