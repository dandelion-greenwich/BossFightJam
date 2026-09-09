// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DeadSignalGameMode.generated.h"

class UHealthComponent;

UENUM(BlueprintType)
enum class EEncounterState : uint8
{
	Intro		UMETA(DisplayName = "Intro"),
	Fighting	UMETA(DisplayName = "Fighting"),
	Victory		UMETA(DisplayName = "Victory"),
	Defeat		UMETA(DisplayName = "Defeat")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEncounterStateChanged, EEncounterState, NewState);

/**
 * Referee for the Hiramor encounter.
 *
 * Deliberately thin. It does not know what phase the boss is in, how the
 * shield works, or what a hack is - that is all boss and player behaviour.
 * It only decides when the fight starts, and which end screen you get.
 *
 * It also does not know the boss's class. It binds through UHealthComponent,
 * so any actor with one can register as the boss.
 */
UCLASS()
class BOSSFIGHTJAM_API ADeadSignalGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADeadSignalGameMode();

	/** Call from the boss's BeginPlay. Binds its death to the victory path. */
	UFUNCTION(BlueprintCallable, Category = "Encounter")
	void RegisterBoss(AActor* InBoss);

	/** Leaves Intro and lets the fight run. Called automatically unless bWaitForManualStart. */
	UFUNCTION(BlueprintCallable, Category = "Encounter")
	void StartEncounter();

	/** Reloads the current level. */
	UFUNCTION(BlueprintCallable, Category = "Encounter")
	void RestartEncounterLevel();

	UFUNCTION(BlueprintPure, Category = "Encounter")
	EEncounterState GetEncounterState() const { return EncounterState; }

	/** True only while the fight is live - use to gate damage, input, boss AI. */
	UFUNCTION(BlueprintPure, Category = "Encounter")
	bool IsFighting() const { return EncounterState == EEncounterState::Fighting; }

	UFUNCTION(BlueprintPure, Category = "Encounter")
	bool IsEncounterOver() const;

	UPROPERTY(BlueprintAssignable, Category = "Encounter")
	FOnEncounterStateChanged OnEncounterStateChanged;

protected:
	virtual void BeginPlay() override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Encounter")
	void OnFightStarted();
	UFUNCTION(BlueprintImplementableEvent, Category = "Encounter")
	void OnVictory();
	UFUNCTION(BlueprintImplementableEvent, Category = "Encounter")
	void OnDefeat();

	/** Leave the encounter in Intro until StartEncounter is called - for an opening cinematic. */
	UPROPERTY(EditDefaultsOnly, Category = "Encounter")
	bool bWaitForManualStart = false;

	/** Seconds between the killing blow and the end screen, so death FX can play. */
	UPROPERTY(EditDefaultsOnly, Category = "Encounter", meta = (ClampMin = "0.0"))
	float EndScreenDelay = 2.f;

	UPROPERTY(BlueprintReadOnly, Category = "Encounter")
	EEncounterState EncounterState = EEncounterState::Intro;

private:
	UFUNCTION()
	void HandlePlayerDeath();

	UFUNCTION()
	void HandleBossDeath();

	void SetEncounterState(EEncounterState NewState);

	/** Binds to an actor's health component if it has one. Returns the component, or null. */
	UHealthComponent* BindDeathHandler(AActor* Actor, bool bIsPlayer);

	UPROPERTY()
	TObjectPtr<AActor> Boss;
	UPROPERTY()
	TObjectPtr<UHealthComponent> BossHealth;
	UPROPERTY()
	TObjectPtr<UHealthComponent> PlayerHealth;
	FTimerHandle EndScreenTimer;
};
