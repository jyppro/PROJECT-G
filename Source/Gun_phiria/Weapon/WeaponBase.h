// WeaponBase.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponBase.generated.h"

class UStaticMeshComponent;
class UNiagaraSystem;

UCLASS()
class GUN_PHIRIA_API AWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	AWeaponBase();

	// 무기 발사 함수 (캐릭터가 타겟 위치를 계산해서 넘겨줍니다)
	virtual void Fire(FVector TargetLocation);

	// 캐릭터가 ADS 조준을 위해 총기 메시 위치를 알아야 할 때 사용
	FORCEINLINE UStaticMeshComponent* GetWeaponMesh() const { return WeaponMesh; }

	// 이 무기의 탄 퍼짐 배수 (권총: 3.0, 소총: 1.5 등 무기마다 다르게 설정 가능)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Stats")
	float WeaponSpreadMultiplier = 3.0f;

	// 이 무기의 HUD 크로스헤어 기본 벌어짐 크기 (권총: 5.0, 소총: 2.0 등)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Stats")
	float WeaponBaseSpreadHUD = 5.0f;

	// 무기의 최소 데미지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Stats")
	float MinWeaponDamage = 10.0f;

	// 무기의 최대 데미지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Stats")
	float MaxWeaponDamage = 20.0f;

protected:
	// 기존 캐릭터 코드에 있던 무기 메시와 이펙트들을 이쪽으로 옮깁니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	TObjectPtr<UNiagaraSystem> MuzzleFlashEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	TObjectPtr<UNiagaraSystem> BulletTracerEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	TObjectPtr<UNiagaraSystem> ImpactEffect;

	// [새로 추가] 적 캐릭터를 맞췄을 때 나올 이펙트 (피 튀김 등)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	TObjectPtr<class UNiagaraSystem> EnemyHitEffect;

	// 탄 퍼짐 관련 변수 (무기마다 퍼짐 정도가 다를 수 있으므로 무기가 가집니다)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float SpreadPerShot = 1.0f;
};