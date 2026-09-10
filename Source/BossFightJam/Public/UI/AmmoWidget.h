#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AmmoWidget.generated.h"

class UWeaponComponent;
class UTextBlock;

/**
 * Base class for WBP_AmmoCounter.
 *
 * Finds the player's weapon and binds its ammo delegates, so Blueprint only
 * has to decide what the readout looks like. Same split as the hack panel:
 * C++ writes the numbers, Blueprint styles them.
 */
UCLASS(Abstract)
class BOSSFIGHTJAM_API UAmmoWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Current / Max, for a radial or a bar OPTIONAL!!!. Returns 0 with no weapon. */
	UFUNCTION(BlueprintPure, Category = "Ammo")
	float GetAmmoFraction() const;

	UFUNCTION(BlueprintPure, Category = "Ammo")
	UWeaponComponent* GetWeaponComponent() const { return WeaponComponent; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/**
	 * Fires whenever the count changes, and once on construction so the readout
	 * starts correct rather than blank.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Ammo")
	void OnAmmoUpdated(int32 CurrentAmmo, int32 MagazineSize);

	UFUNCTION(BlueprintImplementableEvent, Category = "Ammo")
	void OnReloadBegan(float Duration);

	UFUNCTION(BlueprintImplementableEvent, Category = "Ammo")
	void OnReloadEnded();

	/** Trigger the empty-magazine flash here. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Ammo")
	void OnFiredEmpty();
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Ammo")
	TObjectPtr<UTextBlock> AmmoText;

private:
	UFUNCTION()
	void HandleAmmoChanged(int32 CurrentAmmo, int32 MagazineSize);

	UFUNCTION()
	void HandleReloadStarted(float Duration);

	UFUNCTION()
	void HandleReloadFinished();

	UFUNCTION()
	void HandleFiredEmpty();

	UPROPERTY()
	TObjectPtr<UWeaponComponent> WeaponComponent;
};
