// Fill out your copyright notice in the Description page of Project Settings.


#include "DeadSignalGameMode.h"

#include "HealthComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

ADeadSignalGameMode::ADeadSignalGameMode()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ADeadSignalGameMode::BeginPlay()
{
	Super::BeginPlay();

	// The boss registers itself, so it may already have arrived by now.
	if (!bWaitForManualStart)
	{
		StartEncounter();
	}
}

void ADeadSignalGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	// Called once the pawn exists, which BeginPlay cannot guarantee.
	if (NewPlayer)
	{
		PlayerHealth = BindDeathHandler(NewPlayer->GetPawn(), /*bIsPlayer=*/true);
	}
}

void ADeadSignalGameMode::RegisterBoss(AActor* InBoss)
{
	if (!InBoss || Boss == InBoss)
	{
		return;
	}

	Boss = InBoss;
	BossHealth = BindDeathHandler(InBoss, /*bIsPlayer=*/false);
}

UHealthComponent* ADeadSignalGameMode::BindDeathHandler(AActor* Actor, bool bIsPlayer)
{
	if (!Actor)
	{
		return nullptr;
	}

	UHealthComponent* Health = Actor->FindComponentByClass<UHealthComponent>();
	if (!Health)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s has no UHealthComponent - its death will never be detected."),
			*Actor->GetName());
		return nullptr;
	}

	if (bIsPlayer)
	{
		Health->OnDeath.AddDynamic(this, &ADeadSignalGameMode::HandlePlayerDeath);
	}
	else
	{
		Health->OnDeath.AddDynamic(this, &ADeadSignalGameMode::HandleBossDeath);
	}

	return Health;
}

void ADeadSignalGameMode::StartEncounter()
{
	if (EncounterState != EEncounterState::Intro)
	{
		return;
	}

	SetEncounterState(EEncounterState::Fighting);
	OnFightStarted();
}

void ADeadSignalGameMode::HandlePlayerDeath()
{
	if (IsEncounterOver())
	{
		return;
	}

	SetEncounterState(EEncounterState::Defeat);

	// Delay the screen so the death FX reads before the UI covers it.
	FTimerDelegate Delegate;
	Delegate.BindWeakLambda(this, [this]()
	{
		OnDefeat();
	});
	GetWorldTimerManager().SetTimer(EndScreenTimer, Delegate, FMath::Max(EndScreenDelay, KINDA_SMALL_NUMBER), false);
}

void ADeadSignalGameMode::HandleBossDeath()
{
	if (IsEncounterOver())
	{
		return;
	}

	SetEncounterState(EEncounterState::Victory);

	FTimerDelegate Delegate;
	Delegate.BindWeakLambda(this, [this]()
	{
		OnVictory();
	});
	GetWorldTimerManager().SetTimer(EndScreenTimer, Delegate, FMath::Max(EndScreenDelay, KINDA_SMALL_NUMBER), false);
}

void ADeadSignalGameMode::SetEncounterState(EEncounterState NewState)
{
	if (EncounterState == NewState)
	{
		return;
	}

	EncounterState = NewState;
	OnEncounterStateChanged.Broadcast(NewState);
}

bool ADeadSignalGameMode::IsEncounterOver() const
{
	return EncounterState == EEncounterState::Victory || EncounterState == EEncounterState::Defeat;
}

void ADeadSignalGameMode::RestartEncounterLevel()
{
	GetWorldTimerManager().ClearTimer(EndScreenTimer);

	const FName CurrentLevel(*UGameplayStatics::GetCurrentLevelName(this, /*bRemovePrefixString=*/true));
	UGameplayStatics::OpenLevel(this, CurrentLevel);
}
