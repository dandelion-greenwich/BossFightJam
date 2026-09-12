#include "UI/EncounterMessageWidget.h"

#include "BossCharacter.h"
#include "Kismet/GameplayStatics.h"

void UEncounterMessageWidget::NativeConstruct()
{
	Super::NativeConstruct();

	GameMode = Cast<ADeadSignalGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GameMode)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Message] Game mode is not ADeadSignalGameMode - no messages will play."));
		return;
	}

	GameMode->OnEncounterStateChanged.AddDynamic(this, &UEncounterMessageWidget::HandleEncounterStateChanged);
	GameMode->OnBossRegistered.AddDynamic(this, &UEncounterMessageWidget::HandleBossRegistered);

	// The boss may have registered already or may arrive later - its BeginPlay
	// and this widget's construction have no guaranteed order. Cover both.
	BindToBoss(GameMode->GetBossActor());

	// Seeded, because the game mode may already have started the fight in its
	// own BeginPlay and the broadcast we would have heard is gone.
	HandleEncounterStateChanged(GameMode->GetEncounterState());
}

void UEncounterMessageWidget::NativeDestruct()
{
	if (GameMode)
	{
		GameMode->OnEncounterStateChanged.RemoveDynamic(this, &UEncounterMessageWidget::HandleEncounterStateChanged);
		GameMode->OnBossRegistered.RemoveDynamic(this, &UEncounterMessageWidget::HandleBossRegistered);
	}

	if (Boss)
	{
		Boss->OnPhaseChanged.RemoveDynamic(this, &UEncounterMessageWidget::HandlePhaseChanged);
	}

	Super::NativeDestruct();
}

void UEncounterMessageWidget::HandleEncounterStateChanged(EEncounterState NewState)
{
	// Whichever of Intro or Fighting arrives first, and only once.
	if (bIntroPlayed || (NewState != EEncounterState::Intro && NewState != EEncounterState::Fighting))
	{
		return;
	}

	bIntroPlayed = true;

	if (!IntroMessage.IsEmpty())
	{
		PlayMessage(IntroMessage);
	}
}

void UEncounterMessageWidget::HandleBossRegistered(AActor* BossActor)
{
	BindToBoss(BossActor);
}

void UEncounterMessageWidget::BindToBoss(AActor* BossActor)
{
	// Guarded so the delegate firing after we already bound is harmless.
	if (Boss || !BossActor)
	{
		return;
	}

	Boss = Cast<ABossCharacter>(BossActor);
	if (!Boss)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Message] Registered boss %s is not an ABossCharacter - no phase messages."),
			*BossActor->GetName());
		return;
	}

	Boss->OnPhaseChanged.AddDynamic(this, &UEncounterMessageWidget::HandlePhaseChanged);

	UE_LOG(LogTemp, Log, TEXT("[Message] Bound to boss %s - phase messages are live."), *Boss->GetName());

	// Deliberately not seeded with the current phase, unlike the health bar and
	// the music director. Those describe a continuing state; this announces a
	// change, and the opening is the intro's to announce.
}

void UEncounterMessageWidget::HandlePhaseChanged(EBossPhase NewPhase)
{
	const FText* Message = nullptr;

	switch (NewPhase)
	{
	case EBossPhase::Phase2:
		Message = &Phase2Message;
		break;

	case EBossPhase::Phase3:
		Message = &Phase3Message;
		break;

	default:
		break;
	}

	if (!Message || Message->IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Message] Nothing to type for %s - its message is empty."),
			*UEnum::GetValueAsString(NewPhase));
		return;
	}

	PlayMessage(*Message);
}
