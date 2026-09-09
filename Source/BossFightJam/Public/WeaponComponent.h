#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "WeaponComponent.generated.h"

class UCameraComponent;
class UMeshComponent;

UCLASS(ClassGroup = (DeadSignal), meta = (BlueprintSpawnableComponent))
class BOSSFIGHTJAM_API UWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWeaponComponent();
	
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool Fire();

	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool CanFire() const;

	// Seconds until the next shot is allowed. 0 when ready
	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetRemainingCooldown() const;
	
	UFUNCTION(BlueprintPure, Category = "Weapon")
	FVector GetMuzzleLocation() const;

protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Setup",
		meta = (UseComponentPicker, AllowedClasses = "/Script/Engine.MeshComponent"))
	FComponentReference GunMeshReference;

	//S ocket on the gun mesh that muzzle FX originate from.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName MuzzleSocketName = TEXT("Muzzle");

	// Everything visual - muzzle flash, tracer, impact decal, recoil, sound.
	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon")
	void OnFired(const FHitResult& Hit, bool bHitSomething, float DamageDealt);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0.0"))
	float Damage = 25.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "1.0"))
	float Range = 15000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0.0"))
	float FireInterval = 0.12f;

	/** Draws every shot in the world: green missed, red hit, sphere at impact. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Debug")
	bool bDrawDebugTrace = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Debug", meta = (ClampMin = "0.0"))
	float DebugTraceDuration = 2.f;

private:
	UPROPERTY()
	TObjectPtr<UCameraComponent> Camera;
	UPROPERTY()
	TObjectPtr<UMeshComponent> GunMesh;

	// Starts long enough ago that the very first shot always passes the rate gate
	float LastFireTime = -1000.f;
};
