#include "UI/TypewriterWidget.h"

#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UTypewriterWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (bManageVisibility)
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UTypewriterWidget::NativeDestruct()
{
	ClearTimers();

	Super::NativeDestruct();
}

// ------------------------------------------------------------------ Playback

void UTypewriterWidget::PlayMessage(FText Message)
{
	// A new message replaces whatever is in flight rather than queueing behind
	// it. The beats that trigger these are seconds apart, so a queue would only
	// ever delay the message that matters now.
	ClearTimers();

	FullMessage = Message.ToString();
	RevealedCount = 0;
	bTyping = false;

	if (MessageText)
	{
		MessageText->SetText(FText::GetEmpty());
	}

	if (FullMessage.IsEmpty())
	{
		if (bManageVisibility)
		{
			SetVisibility(ESlateVisibility::Collapsed);
		}
		return;
	}

	if (bManageVisibility)
	{
		// HitTestInvisible, not Visible: a message across the screen must not
		// swallow clicks meant for anything underneath it.
		SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	bTyping = true;

	OnMessageStarted();
	ScheduleNextCharacter();
}

void UTypewriterWidget::ClearMessage()
{
	ClearTimers();

	FullMessage.Reset();
	RevealedCount = 0;
	bTyping = false;

	if (MessageText)
	{
		MessageText->SetText(FText::GetEmpty());
	}

	if (bManageVisibility)
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UTypewriterWidget::ScheduleNextCharacter()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	float Delay = 1.f / FMath::Max(1.f, CharactersPerSecond);

	// Measured off the character just revealed, so the beat lands after the
	// comma rather than before it.
	if (RevealedCount > 0)
	{
		const TCHAR Last = FullMessage[RevealedCount - 1];
		const bool bIsPunctuation =
			Last == TEXT('.') || Last == TEXT(',') || Last == TEXT('!') ||
			Last == TEXT('?') || Last == TEXT(':') || Last == TEXT(';');

		if (bIsPunctuation)
		{
			Delay *= PunctuationPauseMultiplier;
		}
	}

	// A world timer rather than NativeTick, for two reasons: the pacing is
	// exact regardless of frame rate, and world timers freeze on pause, where
	// widget tick would keep typing behind the pause menu.
	World->GetTimerManager().SetTimer(TypeTimer, this,
		&UTypewriterWidget::RevealNextCharacter, Delay, false);
}

void UTypewriterWidget::RevealNextCharacter()
{
	++RevealedCount;

	if (MessageText)
	{
		MessageText->SetText(FText::FromString(FullMessage.Left(RevealedCount)));
	}

	OnCharacterRevealed(RevealedCount - 1);

	if (RevealedCount < FullMessage.Len())
	{
		ScheduleNextCharacter();
		return;
	}

	bTyping = false;

	if (HoldDuration > 0.f)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(HoldTimer, this,
				&UTypewriterWidget::FinishMessage, HoldDuration, false);
			return;
		}
	}

	FinishMessage();
}

void UTypewriterWidget::FinishMessage()
{
	// Fired before the collapse so the event can still read the widget as it
	// was, and so a subclass can react before it goes away.
	OnMessageFinished();

	if (bManageVisibility)
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UTypewriterWidget::ClearTimers()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TypeTimer);
		World->GetTimerManager().ClearTimer(HoldTimer);
	}
}
