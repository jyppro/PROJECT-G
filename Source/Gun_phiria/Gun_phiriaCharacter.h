#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Weapon/WeaponBase.h"
#include "Gun_phiriaCharacter.generated.h"

// 전방 선언 (Forward Declarations)
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UStaticMeshComponent;
class UNiagaraSystem;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config = Game)
class AGun_phiriaCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AGun_phiriaCharacter();

	// Public 오버라이드 함수
	virtual void Tick(float DeltaTime) override;

	// =========================================================

	// Getters (외부에서 컴포넌트나 상태를 가져갈 때 사용)
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE float GetCurrentSpread() const { return CurrentSpread; }
	FORCEINLINE class AWeaponBase* GetCurrentWeapon() const { return CurrentWeapon; }

protected:
	// Protected 오버라이드 함수
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// =========================================================

	// 컴포넌트 (Components)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> ADSCamera;

	/*UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> WeaponMesh;*/

	// =========================================================

	// 입력 (Input)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> AimAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> FireAction;

	// =========================================================

	// 입력 처리 함수
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	// =========================================================

	// 전투 및 무기 (Combat & Weapon)
	void StartAiming();
	void StopAiming();
	void Fire();
	void StopFiringPose();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	bool bIsAiming = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	bool bIsFiring = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	float CurrentSpread = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float SpreadPerShot = 1.0f; // 한 발 쏠 때마다 늘어나는 퍼짐 수치

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float MaxSpread = 6.0f; // 퍼짐 한계선 (에임이 끝도 없이 벌어지는 것 방지)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float SpreadRecoveryRate = 5.0f; // 초당 퍼짐 회복(에임이 다시 모이는) 속도

	float SpreadRecoveryDelay = 0.1f;

	// 장착할 기본 무기 클래스 및 현재 무기 포인터
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<AWeaponBase> DefaultWeaponClass;

	UPROPERTY()
	AWeaponBase* CurrentWeapon;

	// =========================================================

	// 카메라 및 조준 세팅 (Camera & Aiming Settings)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aiming")
	FVector DynamicAimOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aiming")
	float AimDistance = 40.0f;

	float DefaultFOV = 90.0f;      // 평소 시야각
	float AimFOV = 60.0f;          // 조준 시 좁아지는 시야각 (확대)
	float ZoomInterpSpeed = 20.0f; // 줌인되는 속도

	// =========================================================

	// 애니메이션 (Animation)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
	float MovementDirectionAngle = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
	float YawRotationSpeed = 0.0f;

	// =========================================================

	// 이펙트 (Effects)
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	//TObjectPtr<UNiagaraSystem> MuzzleFlashEffect; // 총구 화염

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	//TObjectPtr<UNiagaraSystem> BulletTracerEffect; // 궤적

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	//TObjectPtr<UNiagaraSystem> ImpactEffect; // 피격 이펙트

private:
	// 내부 사용 변수 (Internal Use Only)
	FTimerHandle FireTimerHandle;
	float PreviousActorYaw = 0.0f;
	float LastFireTime = 0.0f;
};