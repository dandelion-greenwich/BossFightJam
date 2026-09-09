#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WeaponComponent.generated.h"

class UCameraComponent;
class USceneComponent;

/**
 * The player's gun.
 *
 * Input stays in Blueprint - the pawn binds fire and calls Fire(). The camera
 * and gun mesh are handed in from BP too, since that is where they live.
 *
 * Shots trace from the camera and damage the FIRST thing they hit, so an
 * incoming projectile will body-block a shot aimed at the boss. That is
 * deliberate: clearing bullets is a real tactic rather than a novelty.
 */
UCLASS(ClassGroup = (DeadSignal), meta = (BlueprintSpawnableComponent))
class BOSSFIGHTJAM_API UWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWeaponComponent();

	/** Call from the player BP's BeginPlay. Both references come from BP. */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void Initialize(UCameraComponent* InCamera, USceneComponent* InGunMesh);

	/**
	 * Fire one shot. Call from the BP fire input - holding it cannot outrun
	 * FireInterval, the rate gate lives in here.
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool Fire();

	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool CanFire() const;

	/** Seconds until the next shot is allowed. 0 when ready. */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetRemainingCooldown() const;

	/**
	 * Where tracers and muzzle flash should start. The trace itself comes from
	 * the camera for accuracy; FX come from here so they look right.
	 */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	FVector GetMuzzleLocation() const;

protected:
	/** Everything visual - muzzle flash, tracer, impact decal, recoil, sound. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon")
	void OnFired(const FHitResult& Hit, bool bHitSomething, float DamageDealt);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0.0"))
	float Damage = 25.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "1.0"))
	float Range = 15000.f;

	/** Seconds between shots. 0.12 is roughly 8 shots a second. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0.0"))
	float FireInterval = 0.12f;

	/** Socket on the gun mesh that muzzle FX originate from. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName MuzzleSocketName = TEXT("Muzzle");

private:
	UPROPERTY()
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY()
	TObjectPtr<USceneComponent> GunMesh;

	/** Starts long enough ago that the very first shot always passes the rate gate. */
	float LastFireTime = -1000.f;
};
