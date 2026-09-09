#include "HackComponent.h"

#include "BossCharacter.h"
#include "HealthComponent.h"
#include "DeadSignalGameMode.h"
#include "Kismet/GameplayStatics.h"

UHackComponent::UHackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHackComponent::BeginPlay()
{
	Super::BeginPlay();

	// Sized once from the authored array, so indices can never desync.
	Runtime.SetNum(Hacks.Num());
}

// ---------------------------------------------------------------- Panel

void UHackComponent::TogglePanel()
{
	if (bPanelOpen)
	{
		ClosePanel();
	}
	else
	{
		OpenPanel();
	}
}

void UHackComponent::OpenPanel()
{
	if (bPanelOpen || bInputLocked)
	{
		return;
	}

	bPanelOpen = true;
	ClearInput();
	RegeneratePatterns();

	OnPanelToggled.Broadcast(true);
}

void UHackComponent::ClosePanel()
{
	if (!bPanelOpen)
	{
		return;
	}

	bPanelOpen = false;
	ClearInput();

	OnPanelToggled.Broadcast(false);
}

void UHackComponent::SetInputLocked(bool bNewLocked)
{
	bInputLocked = bNewLocked;

	if (bInputLocked && bPanelOpen)
	{
		ClosePanel();
	}
}

// Patterns

void UHackComponent::RegeneratePatterns()
{
	TArray<TArray<int32>> Chosen;
	Chosen.Reserve(Hacks.Num());

	const int32 Length = FMath::Max(1, SequenceLength);

	for (int32 i = 0; i < Hacks.Num(); ++i)
	{
		const TArray<int32> Previous = Runtime[i].KeySequence;

		// Rejection sampling. With 4^Length combinations and only four hacks
		// this converges immediately; the cap only guards a pathological
		// SequenceLength of 1, where four distinct sequences barely exist.
		TArray<int32> Candidate;
		const int32 MaxAttempts = 200;

		for (int32 Attempt = 0; Attempt < MaxAttempts; ++Attempt)
		{
			Candidate.Reset();
			for (int32 k = 0; k < Length; ++k)
			{
				Candidate.Add(FMath::RandRange(1, 4));
			}

			// Must differ from this hack's previous sequence, and from every
			// sequence already handed out this open. Because all sequences are
			// the same length and distinct, none can be a prefix of another -
			// so the grey-out filter stays unambiguous for free.
			if (Candidate == Previous)
			{
				continue;
			}
			if (Chosen.Contains(Candidate))
			{
				continue;
			}
			break;
		}

		Chosen.Add(Candidate);
		Runtime[i].KeySequence = Candidate;
	}

	OnPatternsRegenerated.Broadcast();
}

// Input

bool UHackComponent::CanAcceptInput() const
{
	if (!bPanelOpen || bInputLocked)
	{
		return false;
	}

	// No hacking before the fight starts or after it ends.
	if (const ADeadSignalGameMode* GameMode = Cast<ADeadSignalGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (!GameMode->IsFighting())
		{
			return false;
		}
	}

	return true;
}

void UHackComponent::SubmitKey(int32 Key)
{
	if (!CanAcceptInput() || Key < 1 || Key > 4)
	{
		return;
	}

	CurrentInput.Add(Key);

	// Which hacks still match what has been typed?
	int32 ExactMatch = INDEX_NONE;
	int32 ViableCount = 0;

	for (int32 i = 0; i < Hacks.Num(); ++i)
	{
		if (!IsHackViable(i))
		{
			continue;
		}

		++ViableCount;

		if (Runtime[i].KeySequence.Num() == CurrentInput.Num())
		{
			ExactMatch = i;
		}
	}

	if (ViableCount == 0)
	{
		FailInput();
		return;
	}

	if (ExactMatch != INDEX_NONE)
	{
		ActivateHack(ExactMatch);
		return;
	}

	OnInputChanged.Broadcast();
}

void UHackComponent::FailInput()
{
	ClearInput();
	OnHackFailed.Broadcast();
}

void UHackComponent::ClearInput()
{
	CurrentInput.Reset();
	OnInputChanged.Broadcast();
}

// Activation

void UHackComponent::ActivateHack(int32 Index)
{
	if (!IsValidIndex(Index) || !GetWorld())
	{
		return;
	}

	const FHackDefinition& Hack = Hacks[Index];
	FHackRuntimeState& State = Runtime[Index];

	ApplyHackEffect(Hack);
	ClearInput();

	OnHackActivated.Broadcast(Hack.Type, Index);

	UE_LOG(LogTemp, Log, TEXT("[Hacks] Activated %s (index %d)"),
		*UEnum::GetValueAsString(Hack.Type), Index);

	if (Hack.Duration > 0.f)
	{
		State.bActive = true;

		FTimerDelegate Expire;
		Expire.BindWeakLambda(this, [this, Index]() { HandleHackExpired(Index); });
		GetWorld()->GetTimerManager().SetTimer(State.DurationTimer, Expire, Hack.Duration, false);
	}
	else
	{
		// Instant hacks such as Heal have nothing to expire, so their cooldown
		// starts right away rather than after a zero-length duration.
		HandleHackExpired(Index);
	}
}

void UHackComponent::HandleHackExpired(int32 Index)
{
	if (!IsValidIndex(Index) || !GetWorld())
	{
		return;
	}

	const FHackDefinition& Hack = Hacks[Index];
	FHackRuntimeState& State = Runtime[Index];

	if (State.bActive)
	{
		State.bActive = false;
		RemoveHackEffect(Hack);
		OnHackExpired.Broadcast(Hack.Type, Index);
	}

	// The cooldown clock starts here rather than
	// at activation, so an active hack does not burn its own downtime
	if (Hack.Cooldown > 0.f)
	{
		FTimerDelegate Ready;
		Ready.BindWeakLambda(this, [this, Index]() { HandleCooldownFinished(Index); });
		GetWorld()->GetTimerManager().SetTimer(State.CooldownTimer, Ready, Hack.Cooldown, false);
	}
}

void UHackComponent::HandleCooldownFinished(int32 Index)
{
	if (!IsValidIndex(Index))
	{
		return;
	}

	OnCooldownFinished.Broadcast(Hacks[Index].Type, Index);
}

// Effects

void UHackComponent::ApplyHackEffect(const FHackDefinition& Hack)
{
	switch (Hack.Type)
	{
	case EHackType::DropShield:
	{
		if (ABossCharacter* Boss = GetBoss())
		{
			Boss->DropShield(Hack.Duration);
		}
		break;
	}

	case EHackType::StunBoss:
	{
		if (ABossCharacter* Boss = GetBoss())
		{
			Boss->ApplyStun(Hack.Duration);
		}
		break;
	}

	case EHackType::PlayerShield:
	{
		if (UHealthComponent* Health = GetOwnerHealth())
		{
			Health->SetInvulnerable(true);
		}
		break;
	}

	case EHackType::Heal:
	{
		if (UHealthComponent* Health = GetOwnerHealth())
		{
			Health->Heal(Hack.HealAmount);
		}
		break;
	}
	}
}

void UHackComponent::RemoveHackEffect(const FHackDefinition& Hack)
{
	// Removes player's shield
	if (Hack.Type == EHackType::PlayerShield)
	{
		if (UHealthComponent* Health = GetOwnerHealth())
		{
			Health->SetInvulnerable(false);
		}
	}
}

// Queries

FHackDefinition UHackComponent::GetHackDefinition(int32 Index) const
{
	return Hacks.IsValidIndex(Index) ? Hacks[Index] : FHackDefinition();
}

TArray<int32> UHackComponent::GetKeySequence(int32 Index) const
{
	return Runtime.IsValidIndex(Index) ? Runtime[Index].KeySequence : TArray<int32>();
}

bool UHackComponent::IsHackViable(int32 Index) const
{
	if (!IsValidIndex(Index) || IsHackActive(Index) || IsOnCooldown(Index))
	{
		return false;
	}

	const TArray<int32>& Sequence = Runtime[Index].KeySequence;
	if (CurrentInput.Num() > Sequence.Num())
	{
		return false;
	}

	for (int32 i = 0; i < CurrentInput.Num(); ++i)
	{
		if (Sequence[i] != CurrentInput[i])
		{
			return false;
		}
	}

	return true;
}

bool UHackComponent::IsHackActive(int32 Index) const
{
	return IsValidIndex(Index) && Runtime[Index].bActive;
}

bool UHackComponent::IsOnCooldown(int32 Index) const
{
	return GetRemainingCooldown(Index) > 0.f;
}

float UHackComponent::GetRemainingCooldown(int32 Index) const
{
	if (!IsValidIndex(Index) || !GetWorld())
	{
		return 0.f;
	}

	return FMath::Max(0.f, GetWorld()->GetTimerManager().GetTimerRemaining(Runtime[Index].CooldownTimer));
}

float UHackComponent::GetRemainingDuration(int32 Index) const
{
	if (!IsValidIndex(Index) || !GetWorld())
	{
		return 0.f;
	}

	return FMath::Max(0.f, GetWorld()->GetTimerManager().GetTimerRemaining(Runtime[Index].DurationTimer));
}

// Lookups

ABossCharacter* UHackComponent::GetBoss()
{
	if (CachedBoss)
	{
		return CachedBoss;
	}

	// Resolved lazily: the boss may register after the player spawns.
	if (const ADeadSignalGameMode* GameMode = Cast<ADeadSignalGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		CachedBoss = Cast<ABossCharacter>(GameMode->GetBossActor());
	}

	if (!CachedBoss)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Hacks] No boss registered - boss-targeting hacks will do nothing."));
	}

	return CachedBoss;
}

UHealthComponent* UHackComponent::GetOwnerHealth() const
{
	const AActor* Owner = GetOwner();
	return Owner ? Owner->FindComponentByClass<UHealthComponent>() : nullptr;
}
