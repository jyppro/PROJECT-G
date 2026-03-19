#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyCharacter.generated.h"

class AWeaponBase;

UCLASS()
class GUN_PHIRIA_API AEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AEnemyCharacter();

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

private:
	// 사격을 반복하기 위한 타이머
	FTimerHandle AIFireTimer;

	// 실제 사격을 수행할 함수
	void FireAtPlayer();
};