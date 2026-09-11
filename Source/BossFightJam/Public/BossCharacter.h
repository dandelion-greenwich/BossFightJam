#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "DeadSignalTypes.h"
#include "Damageable.h"
#include "DeadSignalGameMode.h"
#include "BossCharacter.generated.h"

class UCapsuleComponent;
class USkeletalMeshComponent;
class UHealthComponent;

/** The boss's repertoire. Kept here rather than in the shared types header
 *  because nothing on the player side needs to name an attack. */
UENUM(BlueprintType)
enum class EBossAttackType : uint8
{
	BulletPattern	UMETA(DisplayName = "Bullet Pattern"),
	LaserSweep		UMETA(DisplayName = "Laser Sweep"),
	Teleport		UMETA(DisplayName = "Teleport"),
	EMP				UMETA(DisplayName = "EMP"),
	HackInvasion	UMETA(DisplayName = "Hack Invasion"),
	Pulse			UMETA(DisplayName = "Pulse Knockback")
};

/** One step in a phase's attack order. */
USTRUCT(BlueprintType)
struct FBossAttackStep
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	EBossAttackType Type = EBossAttackType::BulletPattern;

	/** Telegraph before the attack fires, so it can be read and dodged. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.0"))
	float TelegraphTime = 0.75f;

	/** How long the attack itself lasts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.0"))
	float Duration = 1.5f;

	/** Idle time after it finishes, before the next step begins. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.0"))
	float Recovery = 1.f;

	/**
	 * How much of this attack to produce, relative to its base. Lets the same
	 * attack type appear in every phase and simply do more each time.
	 *
	 * Each attack reads it in its own natural way:
	 *   Bullet Pattern - multiplies the bullet count
	 *   Laser Sweep    - number of sweeps
	 *   Teleport       - number of hops
	 *   EMP            - multiplies the blackout duration
	 *   Pulse          - multiplies knockback strength
	 *
	 * 1.0 is the attack's designed baseline, so phase 1 usually leaves it alone.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.1", ClampMax = "10.0"))
	float Intensity = 1.f;

	/**
	 * Intensity as a whole number, for the attacks that repeat rather than
	 * scale - sweeps, teleports. One conversion in one place, so 2.5 cannot
	 * mean three hops in one attack and two in another.
	 */
	int32 GetRepeatCount() const { return FMath::Max(1, FMath::RoundToInt(Intensity)); }
};

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

	/** Begins running the current phase's attack order. Idempotent. */
	UFUNCTION(BlueprintCallable, Category = "Boss|Attacks")
	void StartAttackSequence();

	/** Halts the scheduler. Called when the encounter ends. */
	UFUNCTION(BlueprintCallable, Category = "Boss|Attacks")
	void StopAttackSequence();

	UFUNCTION(BlueprintPure, Category = "Boss|Attacks")
	bool IsAttacking() const { return bSequenceRunning; }

	/** Position in the current phase's sequence, for debugging. */
	UFUNCTION(BlueprintPure, Category = "Boss|Attacks")
	int32 GetCurrentStepIndex() const { return StepIndex; }

	UFUNCTION(BlueprintPure, Category = "Boss|Attacks")
	int32 GetCurrentSequenceLength() const;

#if !UE_BUILD_SHIPPING
	// Debug only 
	void DebugSetPhase(EBossPhase NewPhase);
#endif

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

	/**
	 * Fires when a step begins its telegraph. Play the wind-up here - scale it
	 * by Intensity so a bigger attack looks bigger before it lands.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Attacks")
	void OnAttackTelegraph(EBossAttackType Type, int32 StepIndexInSequence, float Intensity);

	/**
	 * Fires when the attack actually goes off. Spawn projectiles here, reading
	 * Intensity for how many - see FBossAttackStep for what it means per type.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Attacks")
	void OnAttackExecute(EBossAttackType Type, int32 StepIndexInSequence, float Intensity, int32 RepeatCount);

	/** Fires once the whole phase order has been run, before it loops. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Attacks")
	void OnSequenceCompleted(EBossPhase Phase);

	/** The attack order for each phase, run start to finish then looped. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Attacks")
	TArray<FBossAttackStep> Phase1Sequence;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Attacks")
	TArray<FBossAttackStep> Phase2Sequence;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Attacks")
	TArray<FBossAttackStep> Phase3Sequence;

	/** Seconds to wait before retrying when stunned or transitioning. */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attacks", meta = (ClampMin = "0.05"))
	float BlockedRetryInterval = 0.25f;

	/** Prints phase changes, each attack, and sequence completion to the screen. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Debug")
	bool bShowDebugMessages = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss")
	TObjectPtr<UCapsuleComponent> Capsule;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss")
	TObjectPtr<UStaticMeshComponent> Mesh;

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

	UFUNCTION()
	void HandleEncounterStateChanged(EEncounterState NewState);

	/** Runs the step at StepIndex: telegraph, then execute, then schedule next. */
	void RunCurrentStep();
	void ExecuteCurrentStep();
	void AdvanceStep();

	const TArray<FBossAttackStep>& GetSequenceForPhase(EBossPhase Phase) const;
	void ScreenMessage(const FString& Message, const FColor Colour) const;

	bool bTransitioning = false;
	bool bStunned = false;
	bool bSequenceRunning = false;
	int32 StepIndex = 0;

	FTimerHandle ShieldTimer;
	FTimerHandle TransitionTimer;
	FTimerHandle StunTimer;
	FTimerHandle AttackTimer;
};
