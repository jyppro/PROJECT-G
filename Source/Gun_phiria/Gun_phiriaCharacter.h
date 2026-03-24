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

	virtual void Tick(float DeltaTime) override;

	// 데미지 수신 오버라이드 함수
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	// Getters (외부에서 컴포넌트나 상태를 가져갈 때 사용)
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE float GetCurrentSpread() const { return CurrentSpread; }
	FORCEINLINE class AWeaponBase* GetCurrentWeapon() const { return CurrentWeapon; }

	// 조준 상태를 외부(HUD 등)에서 확인할 수 있게 해주는 함수
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool GetIsAiming() const { return bIsAiming; }

	// HUD에서 읽어갈 수 있도록 public에 선언
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crosshair")
	bool bIsAimingAtHead = false;

	// 최대 체력
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MaxHealth = 100.0f;

	// 현재 체력
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float CurrentHealth;

	// 기울이기의 중심축이 될 카메라 시선 벡터
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	FVector LeanAxisCS;

	// 현재 엎드려 있는지 여부
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Prone")
	bool bIsProne;

	// 엎드렸을 때의 최대 이동 속도 (기본 500에서 아주 느리게 설정)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Prone")
	float MaxWalkSpeedProne = 100.0f;

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// 컴포넌트 (Components)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> ADSCamera;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> ProneAction;

	// 입력 처리 함수
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

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

	// 카메라 및 조준 세팅 (Camera & Aiming Settings)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aiming")
	FVector DynamicAimOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aiming")
	float AimDistance = 40.0f;

	float DefaultFOV = 90.0f;      // 평소 시야각
	float AimFOV = 60.0f;          // 조준 시 좁아지는 시야각 (확대)
	float ZoomInterpSpeed = 20.0f; // 줌인되는 속도

	// 애니메이션 (Animation)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
	float MovementDirectionAngle = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
	float YawRotationSpeed = 0.0f;

	// IA_Crouch 입력을 받을 인풋 액션 변수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* CrouchAction;

	void ToggleCrouch();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TArray<class UAnimMontage*> HitMontages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* LeanAction;

	// 목표 기울기 값 (-1.0 ~ 1.0)
	float TargetLean = 0.0f;

	// 현재 부드럽게 보간 중인 기울기 값 (AnimBP로 넘겨줄 변수)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
	float CurrentLean = 0.0f;

	// 키를 누르고 있을 때 실행
	void InputLean(const FInputActionValue& Value);
	// 키를 뗐을 때 실행 (0으로 원상복구)
	void InputLeanEnd(const FInputActionValue& Value);

	// 조준 상태를 0.0 ~ 1.0 사이로 부드럽게 전환하기 위한 알파 값
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aiming")
	float ADSAlpha = 0.0f;

	// 엎드리기 토글 함수
	void ToggleProne();

	// 원래 캡슐 절반 높이와 메쉬의 Z 오프셋을 기억해둘 변수
	float DefaultCapsuleHalfHeight;
	float DefaultMeshRelativeLocationZ;

private:
	FTimerHandle FireTimerHandle;
	float PreviousActorYaw = 0.0f;
	float LastFireTime = 0.0f;

	bool bIsDead = false;
	void Die();
};