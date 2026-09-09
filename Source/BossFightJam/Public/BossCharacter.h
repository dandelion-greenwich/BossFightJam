#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "DeadSignalTypes.h"
#include "Damageable.h"
#include "BossCharacter.generated.h"

class UCapsuleComponent;
class USkeletalMeshComponent;
class UHealthComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPhaseChanged, EBossPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShieldStateChanged, EShieldState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStunChanged, bool, bStunned);

UCLASS()
class BOSSFIGHTJAM_API ABossCharacter : public APawn, public IDamageable
{
	GENERATED_BODY()

public:
	ABossCharacter();

	
	// IDamageable. Forwards to the health component
	virtual float ReceiveShot_Implementation(float Damage, AActor* DamageInstigator, const FHitResult& Hit) override;
	
	UFUNCTION(BlueprintCallable, Category = "Boss|Shield")
	void DropShield(float Duration);

	/** Forces the shield back up immediately, cancelling any active window. */
	UFUNCTION(BlueprintCallable, Category = "Boss|Shield")
	void RestoreShield();

	/** Halts boss behaviour for Duration seconds. Called by the player's hack 2. */
	UFUNCTION(BlueprintCallable, Category = "Boss|Stun")
	void ApplyStun(float Duration);

	UFUNCTION(BlueprintPure, Category = "Boss|Shield")
	EShieldState GetShieldState() const { return ShieldState; }

	UFUNCTION(BlueprintPure, Category = "Boss|Shield")
	bool IsShieldDown() const { return ShieldState == EShieldState::Down; }

	/** Seconds left in the current damage window, 0 if the shield is up. */
	UFUNCTION(BlueprintPure, Category = "Boss|Shield")
	float GetRemainingShieldDownTime() const;

	UFUNCTION(BlueprintPure, Category = "Boss|Phase")
	EBossPhase GetCurrentPhase() const { return CurrentPhase; }

	UFUNCTION(BlueprintPure, Category = "Boss|Phase")
	bool IsTransitioning() const { return bTransitioning; }

	UFUNCTION(BlueprintPure, Category = "Boss|Stun")
	bool IsStunned() const { return bStunned; }

	/** False whenever the boss should not be acting - stunned, transitioning or shut down. */
	UFUNCTION(BlueprintPure, Category = "Boss")
	bool CanAct() const;

	UFUNCTION(BlueprintPure, Category = "Boss")
	UHealthComponent* GetHealthComponent() const { return Health; }

	UPROPERTY(BlueprintAssignable, Category = "Boss|Phase")
	FOnPhaseChanged OnPhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "Boss|Shield")
	FOnShieldStateChanged OnShieldStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Boss|Stun")
	FOnStunChanged OnStunChanged;

protected:
	virtual void BeginPlay() override;

	/** FX hooks for BP_Hiramor. C++ never needs to know what they do. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Shield")
	void OnShieldDropped();
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Shield")
	void OnShieldRestored();
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Phase")
	void OnPhaseTransitionStarted(EBossPhase NewPhase);

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Stun")
	void OnStunStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Stun")
	void OnStunEnded();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss")
	TObjectPtr<UCapsuleComponent> Capsule;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss")
	TObjectPtr<USkeletalMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss")
	TObjectPtr<UHealthComponent> Health;

	/**
	 * Damage scale while the shield holds. 0.2 means shots do a fifth of their
	 * damage - enough that shooting a shielded boss is not pointless, little
	 * enough that dropping the shield is clearly the right play.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Shield", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ShieldedDamageMultiplier = 0.2f;

	/** Health fractions at which phases 2 and 3 begin. */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Phase2Threshold = 0.67f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Phase3Threshold = 0.34f;

	/** Seconds the boss is inert while changing phase. */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase", meta = (ClampMin = "0.0"))
	float TransitionDuration = 2.f;

private:
	UFUNCTION()
	void HandleHealthChanged(float CurrentHealth, float MaxHealth);
	void SetShieldState(EShieldState NewState);
	void EnterPhase(EBossPhase NewPhase);
	void EndTransition();
	void EndStun();

	/** Phase implied by a health fraction, ignoring what phase we are in now. */
	EBossPhase PhaseForHealthPercent(float Percent) const;

	UPROPERTY(VisibleInstanceOnly, Category = "Boss", meta = (AllowPrivateAccess = "true"))
	EBossPhase CurrentPhase = EBossPhase::Phase1;

	UPROPERTY(VisibleInstanceOnly, Category = "Boss", meta = (AllowPrivateAccess = "true"))
	EShieldState ShieldState = EShieldState::Up;

	bool bTransitioning = false;
	bool bStunned = false;
	FTimerHandle ShieldTimer;
	FTimerHandle TransitionTimer;
	FTimerHandle StunTimer;
};
