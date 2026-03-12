#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Gun_phiriaCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config = Game)
class AGun_phiriaCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	// 조준(Aim) 입력 액션
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* AimAction;

	// 사격(Fire) 입력 액션
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* FireAction;

public:
	AGun_phiriaCharacter();

	// 매 프레임 카메라 줌을 부드럽게 처리할 Tick 함수
	virtual void Tick(float DeltaTime) override;

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	// 조준 시작 / 종료 함수
	void StartAiming();
	void StopAiming();

	// 마우스 좌클릭 시 실행될 사격 함수
	void Fire();

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// To add mapping context
	virtual void BeginPlay();

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	// 이 변수가 켜지면 애니메이션 블루프린트에서 조준 자세를 잡습니다!
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	bool bIsAiming = false;

	// 사격 중인지 확인하는 변수
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	bool bIsFiring = false;

	// 반동 및 탄 퍼짐(Spread) 관련 변수들

	// UI(위젯)에서 에임을 벌릴 때 읽어갈 핵심 수치입니다!
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	float CurrentSpread = 0.0f;

protected:
	// 카메라 줌 관련 설정값
	float DefaultFOV = 90.0f; // 평소 시야각
	float AimFOV = 60.0f;     // 조준 시 좁아지는 시야각 (확대)
	float ZoomInterpSpeed = 20.0f; // 줌인되는 속도

	// 타이머를 관리할 핸들과, 사격 자세를 끝내는 함수
	FTimerHandle FireTimerHandle;
	void StopFiringPose();

	// 반동 튜닝용 설정값들 (블루프린트에서 쉽게 수치를 바꿀 수 있게 세팅합니다)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float SpreadPerShot = 1.0f; // 한 발 쏠 때마다 늘어나는 퍼짐 수치

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float MaxSpread = 5.0f; // 퍼짐 한계선 (에임이 끝도 없이 벌어지는 것 방지)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float SpreadRecoveryRate = 5.0f; // 초당 퍼짐 회복(에임이 다시 모이는) 속도

	// 1인칭 정조준(FPS) 전환을 위한 변수들
	// 에디터에서 실시간으로 값을 조절하며 총구에 딱 맞게 세팅할 수 있게 UPROPERTY를 붙입니다!
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float DefaultArmLength = 250.0f; // 평소 카메라 거리 (어깨너머)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float AimArmLength = -10.0f; // 조준 시 카메라 거리 (0 이하면 캐릭터 안으로 바짝 붙습니다)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FVector DefaultSocketOffset = FVector(0.0f, 60.0f, 50.0f); // 평소 위치 (우측 어깨)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FVector AimSocketOffset = FVector(40.0f, 15.0f, 60.0f); // 조준 시 위치 (눈앞/총구 바로 뒤)
};