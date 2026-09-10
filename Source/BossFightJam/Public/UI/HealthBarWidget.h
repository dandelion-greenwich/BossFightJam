#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HealthBarWidget.generated.h"

class UHealthComponent;
class UTextBlock;
class UProgressBar;

/**
 * Base class for WBP_PlayerHealth.
 *
 * Binds the player's health component and pushes changes to Blueprint. The
 * component already exposes everything worth querying, so this deliberately
 * adds no passthrough getters - reach it through GetHealthComponent.
 */
UCLASS(Abstract)
class BOSSFIGHTJAM_API UHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Health")
	UHealthComponent* GetHealthComponent() const { return HealthComponent; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/**
	 * Fires on every change, and once on construction so the bar starts correct
	 * rather than empty. Percent is passed through for convenience since it is
	 * what a progress bar wants.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Health")
	void OnHealthUpdated(float CurrentHealth, float MaxHealth, float Percent);

	/**
	 * A hit landed. This is also the moment hit immunity begins, so it is the
	 * right place to start a damage flash lasting GetRemainingHitImmunity.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Health")
	void OnDamageTaken(float AmountApplied, AActor* DamageInstigator);

	UFUNCTION(BlueprintImplementableEvent, Category = "Health")
	void OnDied();
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Health")
	TObjectPtr<UProgressBar> HealthBar;

	/**
	 * Optional. C++ writes "Current / Max" into it; leave it out and drive the
	 * display from OnHealthUpdated instead.
	 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Health")
	TObjectPtr<UTextBlock> HealthText;

private:
	UFUNCTION()
	void HandleHealthChanged(float CurrentHealth, float MaxHealth);

	UFUNCTION()
	void HandleDamaged(float AmountApplied, AActor* DamageInstigator);

	UFUNCTION()
	void HandleDeath();

	UPROPERTY()
	TObjectPtr<UHealthComponent> HealthComponent;
};
