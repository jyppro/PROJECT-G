#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyCharacter.generated.h"

class AWeaponBase;
class ADungeonRoomManager;
class UAnimMontage;

UCLASS()
class GUN_PHIRIA_API AEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// --- Constructor & Overrides ---
	AEnemyCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	// --- Public API ---
	void FireAtPlayer();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	bool bIsAiming = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation")
	bool bIsCrouching = false;

	// --- Management ---
	UPROPERTY()
	TObjectPtr<ADungeonRoomManager> ParentRoom;

protected:
	// --- Lifecycle ---
	virtual void BeginPlay() override;

	// --- Weapon & Combat ---
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<AWeaponBase> DefaultWeaponClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TObjectPtr<AWeaponBase> CurrentWeapon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float FireRate = 2.0f;

	// --- Animation ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TArray<TObjectPtr<UAnimMontage>> HitMontages;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation")
	float AimPitch = 0.0f;

	// --- Stats ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MaxHealth = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float CurrentHealth;

private:
	// --- Internal Logic ---
	void Die();
	void SetReadyToFire();

	// --- State Flags & Timers ---
	bool bIsDead = false;
	bool bIsReadyToFire = false;

	FTimerHandle SpawnDelayTimer;

};