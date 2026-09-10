#include "UI/HackRowWidget.h"

#include "UI/HackKeyWidget.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"

void UHackRowWidget::SetupRow(int32 InRowIndex, const FHackDefinition& Definition,
	const TArray<int32>& Keys, TSubclassOf<UHackKeyWidget> InKeyWidgetClass)
{
	RowIndex = InRowIndex;
	KeyWidgetClass = InKeyWidgetClass;

	if (NameText)
	{
		NameText->SetText(Definition.DisplayName);
	}

	// Rebuilt from scratch rather than reused: sequence length is a tuning
	// property, so the count can differ from last time.
	//
	// Only the keys we spawned are removed - ClearChildren would also destroy
	// anything hand-placed in the same container, such as a name label.
	for (UHackKeyWidget* Existing : KeyWidgets)
	{
		if (Existing)
		{
			Existing->RemoveFromParent();
		}
	}
	KeyWidgets.Reset();

	if (KeyContainer && KeyWidgetClass)
	{
		for (const int32 Key : Keys)
		{
			// Owned by this widget rather than the player: GetOwningPlayer() is
			// null in the UMG designer, so keys would silently fail to spawn in
			// the preview. Passing the widget works in both contexts.
			UHackKeyWidget* KeyWidget = CreateWidget<UHackKeyWidget>(this, KeyWidgetClass);
			if (!KeyWidget)
			{
				continue;
			}

			KeyContainer->AddChild(KeyWidget);
			// Refresh follows immediately after SetupRow, so this initial state
			// is only what the key shows for a single frame.
			KeyWidget->SetKey(Key, /*bHighlighted=*/false, CurrentState);
			KeyWidgets.Add(KeyWidget);
		}
	}

	OnRowSetup(Definition);
}

void UHackRowWidget::Refresh(EHackRowState State, int32 HighlightedKeys, float CooldownProgress, float ActiveProgress)
{
	CurrentState = State;

	for (int32 i = 0; i < KeyWidgets.Num(); ++i)
	{
		if (KeyWidgets[i])
		{
			KeyWidgets[i]->SetKey(KeyWidgets[i]->GetKeyNumber(), i < HighlightedKeys, State);
		}
	}

	OnRowRefreshed(State, CooldownProgress, ActiveProgress);
}
