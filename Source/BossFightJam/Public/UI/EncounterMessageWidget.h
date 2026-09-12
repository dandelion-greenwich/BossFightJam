#pragma once

#include "CoreMinimal.h"
#include "UI/TypewriterWidget.h"
#include "DeadSignalGameMode.h"
#include "DeadSignalTypes.h"
#include "EncounterMessageWidget.generated.h"

class ABossCharacter;

/**
 * A typewriter that announces the fight.
 *
 * The messages live here rather than on the game mode, which does not know
 * what phase the boss is in and should not learn. Nothing calls this - it
 * binds to the encounter and boss delegates the same way the music director
 * does, so there is no blueprint wiring to get wrong.
 */
UCLASS()
class BOSSFIGHTJAM_API UEncounterMessageWidget : public UTypewriterWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Typed when the fight opens. */
	UPROPERTY(EditDefaultsOnly, Category = "Encounter Message", meta = (MultiLine = true))
	FText IntroMessage;
	
	UPROPERTY(EditDefaultsOnly, Category = "Encounter Message", meta = (MultiLine = true))
	FText Phase2Message;

	UPROPERTY(EditDefaultsOnly, Category = "Encounter Message", meta = (MultiLine = true))
	FText Phase3Message;

private:
	UFUNCTION()
	void HandleEncounterStateChanged(EEncounterState NewState);

	UFUNCTION()
	void HandleBossRegistered(AActor* BossActor);

	UFUNCTION()
	void HandlePhaseChanged(EBossPhase NewPhase);

	void BindToBoss(AActor* BossActor);

	UPROPERTY(Transient)
	TObjectPtr<ADeadSignalGameMode> GameMode;

	UPROPERTY(Transient)
	TObjectPtr<ABossCharacter> Boss;

	/** The opening moment has passed, so Intro and Fighting cannot both fire it. */
	bool bIntroPlayed = false;
};
