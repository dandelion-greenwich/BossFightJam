#include "BossCharacter.h"

#include "HealthComponent.h"
#include "DeadSignalGameMode.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

ABossCharacter::ABossCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	Capsule->SetCapsuleSize(120.f, 250.f);
	Capsule->SetCollisionProfileName(TEXT("Pawn"));

	// For weapon detection
	Capsule->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	RootComponent = Capsule;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Capsule);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Health = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));
}

void ABossCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (Health)
	{
		Health->OnHealthChanged.AddDynamic(this, &ABossCharacter::HandleHealthChanged);

		// The shield starts up, so incoming damage starts reduced.
		Health->SetDamageMultiplier(ShieldedDamageMultiplier);
	}

	// Lets the game mode watch for our death without knowing our type.
	if (ADeadSignalGameMode* GameMode = Cast<ADeadSignalGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GameMode->RegisterBoss(this);
		GameMode->OnEncounterStateChanged.AddDynamic(this, &ABossCharacter::HandleEncounterStateChanged);

		// The encounter may already have started: the game mode calls
		// StartEncounter in its own BeginPlay, which can run before ours.
		HandleEncounterStateChanged(GameMode->GetEncounterState());
	}
}

// ---------------------------------------------------------------- Shield

void ABossCharacter::DropShield(float Duration)
{
	if (Duration <= 0.f)
	{
		return;
	}

	// Restart rather than stack - re-hacking mid-window resets the clock, it
	// does not grant a second window on top of the first.
	GetWorldTimerManager().ClearTimer(ShieldTimer);
	GetWorldTimerManager().SetTimer(ShieldTimer, this, &ABossCharacter::RestoreShield, Duration, false);

	SetShieldState(EShieldState::Down);
}

void ABossCharacter::RestoreShield()
{
	GetWorldTimerManager().ClearTimer(ShieldTimer);
	SetShieldState(EShieldState::Up);
}

void ABossCharacter::SetShieldState(EShieldState NewState)
{
	if (ShieldState == NewState)
	{
		return;
	}

	ShieldState = NewState;

	if (Health)
	{
		Health->SetDamageMultiplier(NewState == EShieldState::Down ? 1.f : ShieldedDamageMultiplier);
	}

	OnShieldStateChanged.Broadcast(NewState);

	if (NewState == EShieldState::Down)
	{
		OnShieldDropped();
	}
	else
	{
		OnShieldRestored();
	}
}

float ABossCharacter::GetRemainingShieldDownTime() const
{
	return FMath::Max(0.f, GetWorldTimerManager().GetTimerRemaining(ShieldTimer));
}


void ABossCharacter::ApplyStun(float Duration)
{
	if (Duration <= 0.f)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(StunTimer);
	GetWorldTimerManager().SetTimer(StunTimer, this, &ABossCharacter::EndStun, Duration, false);

	if (!bStunned)
	{
		bStunned = true;
		OnStunChanged.Broadcast(true);
		OnStunStarted();
	}
}

void ABossCharacter::EndStun()
{
	if (!bStunned)
	{
		return;
	}

	bStunned = false;
	OnStunChanged.Broadcast(false);
	OnStunEnded();
}


void ABossCharacter::HandleHealthChanged(float CurrentHealth, float MaxHealth)
{
	if (MaxHealth <= 0.f)
	{
		return;
	}

	const EBossPhase Target = PhaseForHealthPercent(CurrentHealth / MaxHealth);
	if (Target == CurrentPhase)
	{
		return;
	}

	// Advance one phase at a time even if a burst crossed two thresholds at
	// once - skipping a phase means content nobody ever sees. The next health
	// change re-runs this and steps again if still behind.
	const uint8 Next = static_cast<uint8>(CurrentPhase) + 1;
	if (Next <= static_cast<uint8>(Target))
	{
		EnterPhase(static_cast<EBossPhase>(Next));
	}
}

EBossPhase ABossCharacter::PhaseForHealthPercent(float Percent) const
{
	if (Percent <= Phase3Threshold)
	{
		return EBossPhase::Phase3;
	}
	if (Percent <= Phase2Threshold)
	{
		return EBossPhase::Phase2;
	}
	return EBossPhase::Phase1;
}

void ABossCharacter::EnterPhase(EBossPhase NewPhase)
{
	CurrentPhase = NewPhase;

	// Announce, do not ask. Whoever plays the transition sequence listens; the
	// boss never waits on them, so a missing sequence cannot deadlock the fight.
	OnPhaseChanged.Broadcast(NewPhase);
	OnPhaseTransitionStarted(NewPhase);

	bTransitioning = true;
	GetWorldTimerManager().ClearTimer(TransitionTimer);
	GetWorldTimerManager().SetTimer(TransitionTimer, this, &ABossCharacter::EndTransition,
		FMath::Max(TransitionDuration, 0.001f), false);

	UE_LOG(LogTemp, Log, TEXT("[Boss] Entered phase %d"), static_cast<int32>(NewPhase) + 1);

	ScreenMessage(FString::Printf(TEXT("PHASE %d  (%d attacks)"),
		static_cast<int32>(NewPhase) + 1, GetSequenceForPhase(NewPhase).Num()), FColor::Cyan);

	if (bSequenceRunning)
	{
		// Refresh values for the new phase
		GetWorldTimerManager().ClearTimer(AttackTimer);
		StepIndex = 0;
		RunCurrentStep();
	}
}

void ABossCharacter::EndTransition()
{
	bTransitioning = false;
}

float ABossCharacter::ReceiveShot_Implementation(float Damage, AActor* DamageInstigator, const FHitResult& Hit)
{
	return Health ? Health->ApplyDamage(Damage, DamageInstigator) : 0.f;
}

#if !UE_BUILD_SHIPPING
void ABossCharacter::DebugSetPhase(EBossPhase NewPhase)
{
	if (NewPhase != CurrentPhase)
	{
		EnterPhase(NewPhase);
	}
}
#endif

bool ABossCharacter::CanAct() const
{
	return !bStunned
		&& !bTransitioning
		&& Health
		&& !Health->IsDead();
}

// ---------------------------------------------------------------- Encounter

void ABossCharacter::HandleEncounterStateChanged(EEncounterState NewState)
{
	if (NewState == EEncounterState::Fighting)
	{
		StartAttackSequence();
	}
	else
	{
		// Intro, Victory and Defeat all mean stop - the boss should not be
		// firing during a cinematic or after the end screen.
		StopAttackSequence();
	}
}

// ---------------------------------------------------------------- Attacks

const TArray<FBossAttackStep>& ABossCharacter::GetSequenceForPhase(EBossPhase Phase) const
{
	switch (Phase)
	{
	case EBossPhase::Phase2: return Phase2Sequence;
	case EBossPhase::Phase3: return Phase3Sequence;
	default:                return Phase1Sequence;
	}
}

int32 ABossCharacter::GetCurrentSequenceLength() const
{
	return GetSequenceForPhase(CurrentPhase).Num();
}

void ABossCharacter::StartAttackSequence()
{
	if (bSequenceRunning)
	{
		return;
	}

	if (GetSequenceForPhase(CurrentPhase).Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Boss] Phase %d has an empty attack sequence - the boss will do nothing."),
			static_cast<int32>(CurrentPhase) + 1);
		return;
	}

	bSequenceRunning = true;
	StepIndex = 0;

	RunCurrentStep();
}

void ABossCharacter::StopAttackSequence()
{
	bSequenceRunning = false;
	GetWorldTimerManager().ClearTimer(AttackTimer);
}

void ABossCharacter::RunCurrentStep()
{
	if (!bSequenceRunning)
	{
		return;
	}

	const TArray<FBossAttackStep>& Sequence = GetSequenceForPhase(CurrentPhase);
	if (!Sequence.IsValidIndex(StepIndex))
	{
		StopAttackSequence();
		return;
	}

	// Stunned or mid-transition: wait rather than skip, so a stun costs the
	// boss time instead of silently eating an attack out of the order.
	if (!CanAct())
	{
		GetWorldTimerManager().SetTimer(AttackTimer, this, &ABossCharacter::RunCurrentStep,
			BlockedRetryInterval, false);
		return;
	}

	const FBossAttackStep& Step = Sequence[StepIndex];

	ScreenMessage(FString::Printf(TEXT("P%d  step %d/%d  %s  x%.1f"),
		static_cast<int32>(CurrentPhase) + 1,
		StepIndex + 1, Sequence.Num(),
		*UEnum::GetDisplayValueAsText(Step.Type).ToString(),
		Step.Intensity),
		FColor::Orange);

	OnAttackTelegraph(Step.Type, StepIndex, Step.Intensity);

	if (Step.TelegraphTime > 0.f)
	{
		GetWorldTimerManager().SetTimer(AttackTimer, this, &ABossCharacter::ExecuteCurrentStep,
			Step.TelegraphTime, false);
	}
	else
	{
		ExecuteCurrentStep();
	}
}

void ABossCharacter::ExecuteCurrentStep()
{
	if (!bSequenceRunning)
	{
		return;
	}

	const TArray<FBossAttackStep>& Sequence = GetSequenceForPhase(CurrentPhase);
	if (!Sequence.IsValidIndex(StepIndex))
	{
		StopAttackSequence();
		return;
	}

	const FBossAttackStep& Step = Sequence[StepIndex];

	// The actual attack lives in Blueprint for now - this schedules and
	// announces, the BP spawns whatever the step means.
	OnAttackExecute(Step.Type, StepIndex, Step.Intensity, Step.GetRepeatCount());

	UE_LOG(LogTemp, Log, TEXT("[Boss] t=%.2f  P%d step %d/%d  %s  intensity %.2f (x%d)"),
		GetWorld()->GetTimeSeconds(),
		static_cast<int32>(CurrentPhase) + 1, StepIndex + 1, Sequence.Num(),
		*UEnum::GetValueAsString(Step.Type), Step.Intensity, Step.GetRepeatCount());

	GetWorldTimerManager().SetTimer(AttackTimer, this, &ABossCharacter::AdvanceStep,
		FMath::Max(Step.Duration + Step.Recovery, 0.05f), false);
}

void ABossCharacter::AdvanceStep()
{
	if (!bSequenceRunning)
	{
		return;
	}

	const TArray<FBossAttackStep>& Sequence = GetSequenceForPhase(CurrentPhase);

	++StepIndex;

	if (StepIndex >= Sequence.Num())
	{
		StepIndex = 0;

		ScreenMessage(FString::Printf(TEXT("P%d  sequence complete - looping"),
			static_cast<int32>(CurrentPhase) + 1), FColor::Yellow);

		OnSequenceCompleted(CurrentPhase);
	}

	RunCurrentStep();
}

void ABossCharacter::ScreenMessage(const FString& Message, const FColor Colour) const
{
#if !UE_BUILD_SHIPPING
	if (bShowDebugMessages && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 4.f, Colour, TEXT("[BOSS] ") + Message);
	}
#endif
}
