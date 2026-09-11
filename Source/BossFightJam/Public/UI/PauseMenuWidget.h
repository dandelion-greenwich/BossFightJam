#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PauseMenuWidget.generated.h"

class ADeadSignalGameMode;

UCLASS(Abstract)
class BOSSFIGHTJAM_API UPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Pause")
	void Resume();

	UFUNCTION(BlueprintCallable, Category = "Pause")
	void TogglePause();

	/** Reloads the level. Unpauses first, or the new level starts frozen. */
	UFUNCTION(BlueprintCallable, Category = "Pause")
	void RestartEncounter();

	UFUNCTION(BlueprintCallable, Category = "Pause")
	void QuitGame();

	UFUNCTION(BlueprintPure, Category = "Pause")
	bool IsPaused() const;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;




	/**
	 * Catches the dismiss key while the game is frozen.
	 */
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	
	UPROPERTY(EditDefaultsOnly, Category = "Pause")
	TArray<FKey> ResumeKeys;

	UFUNCTION(BlueprintImplementableEvent, Category = "Pause")
	void OnPaused();

	UFUNCTION(BlueprintImplementableEvent, Category = "Pause")
	void OnResumed();

	/**
	 * Show the cursor and switch to UI input while paused, restoring game input
	 * on resume. Off if you would rather drive input mode from Blueprint.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Pause")
	bool bManageInputMode = true;
	
	UPROPERTY(EditDefaultsOnly, Category = "Pause")
	bool bManageVisibility = true;

private:
	UFUNCTION()
	void HandlePauseChanged(bool bIsPaused);

	ADeadSignalGameMode* GetGameMode() const;

	void ApplyInputMode(bool bIsPaused);
};
