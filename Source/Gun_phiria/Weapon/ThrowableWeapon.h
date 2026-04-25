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

	virtual void Fire(FVector TargetLocation) override;

	UFUNCTION(BlueprintCallable, Category = "Weapon|Throwable")
	void ReleaseThrow(FVector StartLocation, FVector ThrowDirection);

	UFUNCTION(BlueprintCallable, Category = "Weapon|Throwable")
	void CancelThrow();

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Throwable")
	TSubclassOf<AGrenadeProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Throwable")
	float ThrowSpeed = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Throwable")
	float MaxCookTime = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon|Animation")
	TObjectPtr<UAnimMontage> ThrowMontage;

private:
	bool bIsCooking = false;
	float CookStartTime = 0.0f;

	FTimerHandle SwapTimerHandle;

	void ExplodeInHand();
	void ExecutePostThrowSwap();

	// [추가] 중복되는 무기 해제 및 스왑 로직을 처리하는 함수
	void CleanupAndDestroy();
};