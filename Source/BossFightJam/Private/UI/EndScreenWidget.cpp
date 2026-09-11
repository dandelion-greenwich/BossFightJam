#include "UI/EndScreenWidget.h"

#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void UEndScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (bManageVisibility)
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}

	if (ADeadSignalGameMode* GameMode = GetGameMode())
	{
		GameMode->OnGameEnded.AddDynamic(this, &UEndScreenWidget::HandleGameEnded);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[EndScreen] Game mode is not ADeadSignalGameMode - this screen will never show."));
	}
}

void UEndScreenWidget::NativeDestruct()
{
	if (ADeadSignalGameMode* GameMode = GetGameMode())
	{
		GameMode->OnGameEnded.RemoveDynamic(this, &UEndScreenWidget::HandleGameEnded);
	}

	Super::NativeDestruct();
}

ADeadSignalGameMode* UEndScreenWidget::GetGameMode() const
{
	return Cast<ADeadSignalGameMode>(UGameplayStatics::GetGameMode(this));
}

void UEndScreenWidget::HandleGameEnded(EEncounterState FinalState)
{
	// Both screens hear every ending; only the matching one reacts.
	if (FinalState != GetTriggerState())
	{
		return;
	}

	if (bManageVisibility)
	{
		// Before the event, so an entry animation plays against a widget that
		// is already on screen.
		SetVisibility(ESlateVisibility::Visible);
	}

	if (bManageInputMode)
	{
		if (APlayerController* PC = GetOwningPlayer())
		{
			// UIOnly: the fight is over, so nothing should reach the game, and
			// clicks cannot fall through to the viewport and capture the mouse.
			FInputModeUIOnly Mode;
			Mode.SetWidgetToFocus(TakeWidget());
			Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PC->SetInputMode(Mode);
			PC->SetShowMouseCursor(true);
		}
	}

	OnShown();
}

void UEndScreenWidget::RestartEncounter()
{
	if (ADeadSignalGameMode* GameMode = GetGameMode())
	{
		// Clears the dilation and the pause flag before reloading, so the new
		// level does not start slowed or frozen.
		GameMode->RestartEncounterLevel();
	}
}
