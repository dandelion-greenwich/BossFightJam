#include "UI/HackPanelWidget.h"

#include "HackComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

void UHackPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindToHackComponent();

	if (HackComponent)
	{
		OnPanelReady();
	}
}

void UHackPanelWidget::NativeDestruct()
{
	// The component outlives the widget when the HUD is rebuilt, so leaving
	// these bound would call into a dead widget.
	if (HackComponent)
	{
		HackComponent->OnPanelToggled.RemoveDynamic(this, &UHackPanelWidget::HandlePanelToggled);
		HackComponent->OnPatternsRegenerated.RemoveDynamic(this, &UHackPanelWidget::HandlePatternsRegenerated);
		HackComponent->OnInputChanged.RemoveDynamic(this, &UHackPanelWidget::HandleInputChanged);
		HackComponent->OnHackFailed.RemoveDynamic(this, &UHackPanelWidget::HandleHackFailed);
		HackComponent->OnHackActivated.RemoveDynamic(this, &UHackPanelWidget::HandleHackActivated);
		HackComponent->OnHackExpired.RemoveDynamic(this, &UHackPanelWidget::HandleHackExpired);
		HackComponent->OnCooldownFinished.RemoveDynamic(this, &UHackPanelWidget::HandleCooldownFinished);
	}

	Super::NativeDestruct();
}

void UHackPanelWidget::BindToHackComponent()
{
	const APlayerController* PC = GetOwningPlayer();
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;

	if (!Pawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HackPanel] No owning pawn - the panel will show nothing."));
		return;
	}

	HackComponent = Pawn->FindComponentByClass<UHackComponent>();

	if (!HackComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HackPanel] %s has no UHackComponent."), *Pawn->GetName());
		return;
	}

	HackComponent->OnPanelToggled.AddDynamic(this, &UHackPanelWidget::HandlePanelToggled);
	HackComponent->OnPatternsRegenerated.AddDynamic(this, &UHackPanelWidget::HandlePatternsRegenerated);
	HackComponent->OnInputChanged.AddDynamic(this, &UHackPanelWidget::HandleInputChanged);
	HackComponent->OnHackFailed.AddDynamic(this, &UHackPanelWidget::HandleHackFailed);
	HackComponent->OnHackActivated.AddDynamic(this, &UHackPanelWidget::HandleHackActivated);
	HackComponent->OnHackExpired.AddDynamic(this, &UHackPanelWidget::HandleHackExpired);
	HackComponent->OnCooldownFinished.AddDynamic(this, &UHackPanelWidget::HandleCooldownFinished);
}

// ---------------------------------------------------------------- Row state

EHackRowState UHackPanelWidget::GetRowState(int32 Index) const
{
	if (!HackComponent)
	{
		return EHackRowState::Available;
	}

	// Active and cooling win over anything typed - in both cases the player
	// cannot start this hack, whatever the input says.
	if (HackComponent->IsHackActive(Index))
	{
		return EHackRowState::Active;
	}

	if (HackComponent->IsOnCooldown(Index))
	{
		return EHackRowState::Cooling;
	}

	// Nothing typed yet: every usable row is simply available, not "matching".
	if (HackComponent->GetCurrentInput().Num() == 0)
	{
		return EHackRowState::Available;
	}

	return HackComponent->IsHackViable(Index) ? EHackRowState::Matching : EHackRowState::Filtered;
}

int32 UHackPanelWidget::GetHighlightedKeyCount(int32 Index) const
{
	if (!HackComponent || !HackComponent->IsHackViable(Index))
	{
		return 0;
	}

	// Viable means the typed keys are a prefix of this sequence, so the count
	// of entered keys is exactly how many of this row's glyphs light up.
	return HackComponent->GetCurrentInput().Num();
}

// ---------------------------------------------------------------- Passthrough

int32 UHackPanelWidget::GetHackCount() const
{
	return HackComponent ? HackComponent->GetHackCount() : 0;
}

FHackDefinition UHackPanelWidget::GetHackDefinition(int32 Index) const
{
	return HackComponent ? HackComponent->GetHackDefinition(Index) : FHackDefinition();
}

TArray<int32> UHackPanelWidget::GetKeySequence(int32 Index) const
{
	return HackComponent ? HackComponent->GetKeySequence(Index) : TArray<int32>();
}

bool UHackPanelWidget::IsPanelOpen() const
{
	return HackComponent && HackComponent->IsPanelOpen();
}

float UHackPanelWidget::GetCooldownProgress(int32 Index) const
{
	if (!HackComponent)
	{
		return 0.f;
	}

	const float Cooldown = HackComponent->GetHackDefinition(Index).Cooldown;
	if (Cooldown <= 0.f)
	{
		return 0.f;
	}

	const float Remaining = HackComponent->GetRemainingCooldown(Index);
	return FMath::Clamp(1.f - (Remaining / Cooldown), 0.f, 1.f);
}

float UHackPanelWidget::GetActiveProgress(int32 Index) const
{
	if (!HackComponent)
	{
		return 0.f;
	}

	const float Duration = HackComponent->GetHackDefinition(Index).Duration;
	if (Duration <= 0.f)
	{
		return 0.f;
	}

	const float Remaining = HackComponent->GetRemainingDuration(Index);
	return FMath::Clamp(1.f - (Remaining / Duration), 0.f, 1.f);
}

// ---------------------------------------------------------------- Handlers

void UHackPanelWidget::HandlePanelToggled(bool bIsOpen)
{
	OnPanelOpenChanged(bIsOpen);
}

void UHackPanelWidget::HandlePatternsRegenerated()
{
	OnPatternsChanged();
}

void UHackPanelWidget::HandleInputChanged()
{
	OnInputChanged();
}

void UHackPanelWidget::HandleHackFailed()
{
	OnHackFailed();
}

void UHackPanelWidget::HandleHackActivated(EHackType Type, int32 Index)
{
	OnHackActivated(Type, Index);
}

void UHackPanelWidget::HandleHackExpired(EHackType Type, int32 Index)
{
	OnHackExpired(Type, Index);
}

void UHackPanelWidget::HandleCooldownFinished(EHackType Type, int32 Index)
{
	OnCooldownFinished(Type, Index);
}
