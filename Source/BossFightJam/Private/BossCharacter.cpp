#include "BossCharacter.h"

#include "HealthComponent.h"
#include "DeadSignalGameMode.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"

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
