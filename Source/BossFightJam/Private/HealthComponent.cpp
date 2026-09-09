#include "HealthComponent.h"

UHealthComponent::UHealthComponent()
{
	// Nothing here needs to tick - everything is event driven.
	PrimaryComponentTick.bCanEverTick = false;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;
	bIsDead = false;

	// Fire once so any UI bound before BeginPlay starts with the right value.
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

float UHealthComponent::ApplyDamage(float Amount, AActor* DamageInstigator)
{
	if (bIsDead || bIsInvulnerable || Amount <= 0.f)
	{
		return 0.f;
	}

	const float Scaled = Amount * DamageMultiplier;
	if (Scaled <= 0.f)
	{
		return 0.f;
	}

	// Never subtract more than is left, so the reported figure matches reality.
	const float Applied = FMath::Min(Scaled, CurrentHealth);
	CurrentHealth -= Applied;

	OnDamaged.Broadcast(Applied, DamageInstigator);
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);

	if (CurrentHealth <= 0.f)
	{
		bIsDead = true;
		OnDeath.Broadcast();
	}

	return Applied;
}

float UHealthComponent::Heal(float Amount)
{
	if (bIsDead || Amount <= 0.f)
	{
		return 0.f;
	}

	const float Restored = FMath::Min(Amount, MaxHealth - CurrentHealth);
	if (Restored <= 0.f)
	{
		return 0.f;
	}

	CurrentHealth += Restored;
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);

	return Restored;
}

void UHealthComponent::SetHealth(float NewHealth)
{
	const float Clamped = FMath::Clamp(NewHealth, 0.f, MaxHealth);
	if (FMath::IsNearlyEqual(Clamped, CurrentHealth))
	{
		return;
	}

	CurrentHealth = Clamped;
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);

	if (CurrentHealth <= 0.f && !bIsDead)
	{
		bIsDead = true;
		OnDeath.Broadcast();
	}
}

void UHealthComponent::ResetHealth()
{
	CurrentHealth = MaxHealth;
	bIsDead = false;
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void UHealthComponent::SetDamageMultiplier(float NewMultiplier)
{
	DamageMultiplier = FMath::Max(0.f, NewMultiplier);
}
