#include "WeaponComponent.h"

#include "Damageable.h"
#include "HealthComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"

UWeaponComponent::UWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWeaponComponent::Initialize(UCameraComponent* InCamera, USceneComponent* InGunMesh)
{
	Camera = InCamera;
	GunMesh = InGunMesh;

	if (!Camera)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Weapon] Initialized without a camera - Fire will do nothing."));
	}
}

bool UWeaponComponent::CanFire() const
{
	if (!Camera || !GetWorld())
	{
		return false;
	}

	// A dead player should not keep shooting.
	if (const AActor* Owner = GetOwner())
	{
		if (const UHealthComponent* Health = Owner->FindComponentByClass<UHealthComponent>())
		{
			if (Health->IsDead())
			{
				return false;
			}
		}
	}

	return GetWorld()->GetTimeSeconds() - LastFireTime >= FireInterval;
}

float UWeaponComponent::GetRemainingCooldown() const
{
	if (!GetWorld())
	{
		return 0.f;
	}

	const float Elapsed = GetWorld()->GetTimeSeconds() - LastFireTime;
	return FMath::Max(0.f, FireInterval - Elapsed);
}

bool UWeaponComponent::Fire()
{
	if (!CanFire())
	{
		return false;
	}

	LastFireTime = GetWorld()->GetTimeSeconds();

	// From the camera, not the muzzle - what the crosshair covers is what gets hit.
	const FVector Start = Camera->GetComponentLocation();
	const FVector End = Start + Camera->GetForwardVector() * Range;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(WeaponFire), /*bTraceComplex=*/true);
	Params.AddIgnoredActor(GetOwner());

	// LineTraceSingle returns the first blocking hit, which is exactly the
	// "damage the first object" rule - no sorting needed.
	FHitResult Hit;
	const bool bHitSomething = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);

	float DamageDealt = 0.f;

	if (bHitSomething)
	{
		if (AActor* HitActor = Hit.GetActor())
		{
			if (HitActor->Implements<UDamageable>())
			{
				DamageDealt = IDamageable::Execute_ReceiveShot(HitActor, Damage, GetOwner(), Hit);
			}
		}
	}

	OnFired(Hit, bHitSomething, DamageDealt);

	return true;
}

FVector UWeaponComponent::GetMuzzleLocation() const
{
	if (GunMesh)
	{
		if (GunMesh->DoesSocketExist(MuzzleSocketName))
		{
			return GunMesh->GetSocketLocation(MuzzleSocketName);
		}

		return GunMesh->GetComponentLocation();
	}

	return Camera ? Camera->GetComponentLocation() : FVector::ZeroVector;
}
