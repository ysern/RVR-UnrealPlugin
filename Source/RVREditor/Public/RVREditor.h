// Copyright 2026, Iurii Sernivka.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FSlateStyleSet;
class SWindow;

/**
 * The Editor's side of RVR: the toolbar button, the Status Dialog, and VR Preview
 * following the Review Session. A shell over rvr-host-api: it shows what
 * the library reports, passes the Operator's commands on, and starts and stops VR
 * Preview when the library says to. It decides nothing else.
 */
class FRVREditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RegisterToolbar();
	void OpenStatusDialog();
	void OnStatusChanged();
	void AnswerStartRequest();
	void FollowShouldRunVr();
	void OnMapChange(uint32 Flags);
	void OnEditorInitialized(double Duration);
	/** Opens the Host Session asked for at start-up, or gives the one this Editor holds its new level. */
	void DescribeHostSession();

	TSharedPtr<FSlateStyleSet> Style;
	TWeakPtr<SWindow> Dialog;
	FDelegateHandle StatusHandle;
	FDelegateHandle MapChangeHandle;
	FDelegateHandle EditorInitializedHandle;
	bool bInVr = false;
	bool bOpenHostSessionWhenLevelIsKnown = false;
};
