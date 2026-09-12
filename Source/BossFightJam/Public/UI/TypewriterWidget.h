#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TypewriterWidget.generated.h"

class UTextBlock;

/**
 * Reveals text one character at a time.
 *
 * Knows nothing about the encounter - it types whatever it is handed, so the
 * same widget serves an intro, a phase announcement, or anything later.
 * UEncounterMessageWidget is the subclass that wires itself to the fight.
 */
UCLASS()
class BOSSFIGHTJAM_API UTypewriterWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Start revealing. Replaces whatever was on screen. */
	UFUNCTION(BlueprintCallable, Category = "Typewriter")
	void PlayMessage(FText Message);

	/** Blank the text and stop. */
	UFUNCTION(BlueprintCallable, Category = "Typewriter")
	void ClearMessage();

	UFUNCTION(BlueprintPure, Category = "Typewriter")
	bool IsTyping() const { return bTyping; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MessageText;

	UPROPERTY(EditDefaultsOnly, Category = "Typewriter", meta = (ClampMin = "1.0"))
	float CharactersPerSecond = 30.f;

	/**
	 * How much longer the pause is after . , ! ? : ; as a multiple of the base
	 * interval. 1 types straight through. This is most of what separates
	 * "typing" from "characters appearing".
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Typewriter", meta = (ClampMin = "1.0"))
	float PunctuationPauseMultiplier = 6.f;

	/** Seconds the finished message holds before OnMessageFinished. */
	UPROPERTY(EditDefaultsOnly, Category = "Typewriter", meta = (ClampMin = "0.0"))
	float HoldDuration = 2.5f;

	/**
	 * Collapse while there is nothing to say, show while typing.
	 *
	 * Turn it off if you want an exit animation - otherwise the collapse in
	 * OnMessageFinished cuts the animation off on the same frame it starts.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Typewriter")
	bool bManageVisibility = true;

	UFUNCTION(BlueprintImplementableEvent, Category = "Typewriter")
	void OnMessageStarted();

	/** One character just appeared. Hook the typing blip here. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Typewriter")
	void OnCharacterRevealed(int32 Index);

	/** The hold has elapsed. Play the exit animation here. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Typewriter")
	void OnMessageFinished();

private:
	void ScheduleNextCharacter();
	void RevealNextCharacter();
	void FinishMessage();
	void ClearTimers();

	FString FullMessage;
	int32 RevealedCount = 0;
	bool bTyping = false;

	FTimerHandle TypeTimer;
	FTimerHandle HoldTimer;
};
