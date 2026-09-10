#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/HackPanelWidget.h"
#include "HackKeyWidget.generated.h"

class UTextBlock;

/**
 * One key glyph in a hack sequence.
 *
 * Its own widget rather than one character of a shared text block, because
 * each key lights independently as the player types - and a per-key widget can
 * carry a background, a scale change or an animation, none of which a single
 * text block can do per character.
 *
 * C++ sets the number. Blueprint decides what lit and unlit look like.
 */
UCLASS(Abstract)
class BOSSFIGHTJAM_API UHackKeyWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Called by the row. Sets the number, then hands styling to Blueprint. */
	void SetKey(int32 InKeyNumber, bool bInHighlighted, EHackRowState InRowState);

	UFUNCTION(BlueprintPure, Category = "Hacks|Key")
	int32 GetKeyNumber() const { return KeyNumber; }

	UFUNCTION(BlueprintPure, Category = "Hacks|Key")
	bool IsHighlighted() const { return bHighlighted; }

	/** The owning row's state, so a key can dim with a cooling row. */
	UFUNCTION(BlueprintPure, Category = "Hacks|Key")
	EHackRowState GetRowState() const { return RowState; }

protected:
	/**
	 * Do the colouring here - or play an animation. Fires whenever either value
	 * changes, including the first time the key is filled in.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Hacks|Key")
	void OnKeyStateChanged(int32 InKeyNumber, bool bInHighlighted, EHackRowState InRowState);

	/** Optional - draw the number some other way and C++ will not mind. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Hacks|Key")
	TObjectPtr<UTextBlock> KeyText;

private:
	int32 KeyNumber = 0;
	bool bHighlighted = false;
	EHackRowState RowState = EHackRowState::Available;
};
