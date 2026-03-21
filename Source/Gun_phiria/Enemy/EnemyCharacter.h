#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "EnemyCharacter.generated.h"

class AWeaponBase;
class ADungeonRoomManager;

UCLASS()
class GUN_PHIRIA_API AEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AEnemyCharacter();

	virtual void Tick(float DeltaTime) override;

	// 실제 사격을 수행할 함수
	void FireAtPlayer();

protected:
	virtual void BeginPlay() override;

public:
	// 적이 시작할 때 장착할 기본 무기 (블루프린트에서 권총, 소총 등을 자유롭게 할당)
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<AWeaponBase> DefaultWeaponClass;

	// 현재 손에 들고 있는 무기
	UPROPERTY()
	AWeaponBase* CurrentWeapon;

	// 사격 간격 (예: 2초마다 1발씩 발사)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float FireRate = 2.0f;

	// 최대 체력
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MaxHealth = 100.0f;

	// 현재 체력
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float CurrentHealth;

	// 언리얼 내장 데미지 처리 함수 오버라이드 (누군가 ApplyDamage를 호출하면 이게 실행됨)
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	UPROPERTY()
	ADungeonRoomManager* ParentRoom;

	// 애니메이션 블루프린트에서 "지금 조준 자세를 취해야 하는지" 읽어갈 변수
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation")
	bool bIsAiming = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TArray<class UAnimMontage*> HitMontages;

	// 애니메이션 블루프린트로 넘겨줄 위/아래 쳐다보는 각도 (Pitch)
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation")
	float AimPitch = 0.0f;

	// AI가 지금 숙여야 하는지 여부 (나중에 비헤이비어 트리에서 제어)
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation")
	bool bIsCrouching = false;

private:

	// 사망 처리 함수
	void Die();

	// 죽었는지 여부 확인
	bool bIsDead = false;

	// 스폰 직후 방아쇠가 잠겨있는지 확인하는 변수
	bool bIsReadyToFire = false;

	// 사격 대기 시간을 재기 위한 타이머
	FTimerHandle SpawnDelayTimer;

	// 방아쇠 잠금을 해제하는 함수
	void SetReadyToFire();
};