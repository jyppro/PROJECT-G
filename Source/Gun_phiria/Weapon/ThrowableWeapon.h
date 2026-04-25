#pragma once

#include "CoreMinimal.h"
#include "WeaponBase.h"
#include "ThrowableWeapon.generated.h"

class AGrenadeProjectile;
class UAnimMontage;

UCLASS()
class GUN_PHIRIA_API AThrowableWeapon : public AWeaponBase
{
	GENERATED_BODY()

public:
	AThrowableWeapon();

	// AWeaponBase의 Fire를 오버라이드하여 히트스캔 대신 쿠킹/투척 로직을 넣습니다.
	virtual void Fire(FVector TargetLocation) override;

	// 마우스를 뗐을 때 투척을 실행하는 함수
	UFUNCTION(BlueprintCallable, Category = "Weapon|Throwable")
	void ReleaseThrow(FVector StartLocation, FVector ThrowDirection);

	// 쿠킹(핀 뽑기) 취소
	UFUNCTION(BlueprintCallable, Category = "Weapon|Throwable")
	void CancelThrow();

	// 폭발할 발사체 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Throwable")
	TSubclassOf<AGrenadeProjectile> ProjectileClass;

	// 던지는 힘 (속도)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Throwable")
	float ThrowSpeed = 2000.0f;

	// 수류탄 쿠킹 제한 시간 (보통 5초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Throwable")
	float MaxCookTime = 5.0f;

	// 투척 애니메이션
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon|Animation")
	TObjectPtr<UAnimMontage> ThrowMontage;

protected:
	virtual void BeginPlay() override;

private:
	// 상태 변수
	bool bIsCooking = false;

	// 쿠킹 시작 시간 기록
	float CookStartTime = 0.0f;

	// 수류탄 자체 폭발 타이머 (손에 쥐고 터질 때 대비)
	FTimerHandle CookTimerHandle;

	// 손에서 터지는 자폭 함수
	void ExplodeInHand();

	FTimerHandle SwapTimerHandle;

	// 수류탄 투척 애니메이션 종료 후 실행될 스왑 및 삭제 함수
	void ExecutePostThrowSwap();
};