#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponBase.generated.h"

// --- Forward Declarations ---
class UStaticMeshComponent;
class UNiagaraSystem;
class UAnimMontage;

UCLASS()
class GUN_PHIRIA_API AWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	// --- Constructor & API ---
	AWeaponBase();
	virtual void Fire(FVector TargetLocation);

	// --- Getters ---
	FORCEINLINE TObjectPtr<UStaticMeshComponent> GetWeaponMesh() const { return WeaponMesh; }

	// --- Weapon Stats ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Stats")
	float WeaponSpreadMultiplier = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Stats")
	float WeaponBaseSpreadHUD = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Stats")
	float MinWeaponDamage = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Stats")
	float MaxWeaponDamage = 20.0f;

	// --- Animation ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon|Animation")
	TArray<TObjectPtr<UAnimMontage>> StandCrouchFireMontages;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon|Animation")
	TArray<TObjectPtr<UAnimMontage>> ProneFireMontages;

protected:
	// --- Components ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Components")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	// --- Effects ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Effects")
	TObjectPtr<UNiagaraSystem> MuzzleFlashEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Effects")
	TObjectPtr<UNiagaraSystem> BulletTracerEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Effects")
	TObjectPtr<UNiagaraSystem> ImpactEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Effects")
	TObjectPtr<UNiagaraSystem> EnemyHitEffect;

	// --- Combat Settings ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Combat")
	float SpreadPerShot = 1.0f;

};