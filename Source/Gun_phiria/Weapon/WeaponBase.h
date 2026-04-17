#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponBase.generated.h"

// --- Forward Declarations ---
class UStaticMeshComponent;
class UNiagaraSystem;
class UAnimMontage;

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	Pistol  UMETA(DisplayName = "Pistol"),
	SMG     UMETA(DisplayName = "SMG"),
	AR      UMETA(DisplayName = "AR"),
	SR	    UMETA(DisplayName = "SR"),
	Shotgun UMETA(DisplayName = "Shotgun")
};

UENUM(BlueprintType)
enum class EFireMode : uint8
{
	Single UMETA(DisplayName = "Single"),
	Burst  UMETA(DisplayName = "Burst"),
	Auto   UMETA(DisplayName = "Auto")
};

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Holster")
	FRotator HolsterRotationOffset = FRotator::ZeroRotator;

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

	// --- Weapon Info ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon|Info")
	EWeaponType WeaponType = EWeaponType::AR;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon|Info")
	EFireMode FireMode = EFireMode::Single;

	// --- Ammo System ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Ammo")
	int32 CurrentAmmo = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Ammo")
	int32 MagazineCapacity = 30;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Ammo")
	bool bInfiniteAmmo = false;

	// --- Fire Control ---
	// 분당 발사 속도 (RPM) - 예: M416은 보통 700~800
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Stats")
	float FireRate = 600.0f;

	// --- Audio ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Audio")
	TObjectPtr<class USoundBase> FireSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Audio")
	TObjectPtr<class USoundBase> EmptyMagSound; // 총알 없을 때 '찰칵' 소리

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon|Ammo")
	FName AmmoItemID;

	// 재장전할 때 재생할 애니메이션 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon|Animation")
	TObjectPtr<class UAnimMontage> ReloadMontage;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Effects")
	TObjectPtr<UNiagaraSystem> ShellEjectEffect;

	// 탄피가 튀어나올 소켓 이름
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon|Effects")
	FName EjectSocketName = TEXT("AmmoEjectSocket");

	// --- Combat Settings ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Combat")
	float SpreadPerShot = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Components")
	TObjectPtr<class USceneComponent> RootComp;

};