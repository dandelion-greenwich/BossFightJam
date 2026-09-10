#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "Debug/SDebugPanel.h"
#include "Debug/DebugControlLibrary.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"

namespace DeadSignalDebug
{
	/** Weak so the window can be closed normally without leaking a reference. */
	TWeakPtr<SWindow> ActiveWindow;

	void OpenPanel()
	{
		if (!FSlateApplication::IsInitialized())
		{
			UE_LOG(LogTemp, Warning, TEXT("[Debug] Slate is not available - cannot open the panel."));
			return;
		}

		// Already open: bring it forward rather than stacking a second copy.
		if (const TSharedPtr<SWindow> Existing = ActiveWindow.Pin())
		{
			Existing->BringToFront();
			return;
		}

		const TSharedRef<SWindow> Window = SNew(SWindow)
			.Title(NSLOCTEXT("DebugPanel", "Title", "DEAD//SIGNAL Debug"))
			.ClientSize(FVector2D(420.f, 640.f))
			.SupportsMaximize(false)
			.SupportsMinimize(false)
			[
				SNew(SDebugPanel)
			];

		// A top-level window rather than a child, so it can be dragged outside
		// the editor entirely - onto a second monitor if you want.
		FSlateApplication::Get().AddWindow(Window);
		ActiveWindow = Window;
	}
}

static FAutoConsoleCommand DebugPanelCommand(
	TEXT("Debug.Panel"),
	TEXT("Opens the DEAD//SIGNAL debug window."),
	FConsoleCommandDelegate::CreateStatic(&DeadSignalDebug::OpenPanel));

static FAutoConsoleCommand DebugStatusCommand(
	TEXT("Debug.Status"),
	TEXT("Logs a one-shot summary of player, boss and hack state."),
	FConsoleCommandDelegate::CreateLambda([]()
	{
		UE_LOG(LogTemp, Log, TEXT("%s"), *UDebugControlLibrary::GetStatusSummary());
	}));

#endif
