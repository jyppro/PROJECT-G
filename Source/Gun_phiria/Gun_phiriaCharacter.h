#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Gun_phiriaCharacter.generated.h"

// --- Forward Declarations ---
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class AWeaponBase;
class UAnimMontage;
struct FInputActionValue;
class UCastBarWidget;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config = Game)
class GUN_PHIRIA_API AGun_phiriaCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// --- Constructor & Overrides ---
	AGun_phiriaCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	// --- Getters & Public API ---
	FORCEINLINE TObjectPtr<USpringArmComponent> GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE TObjectPtr<UCameraComponent> GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE float GetCurrentSpread() const { return CurrentSpread; }
	FORCEINLINE TObjectPtr<AWeaponBase> GetCurrentWeapon() const { return CurrentWeapon; }

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool GetIsAiming() const { return bIsAiming; }

	// --- Public Stats (HUD / UI) ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats|State")
	bool bIsAimingAtHead = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MaxHealth = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float CurrentHealth;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	FVector LeanAxisCS;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Prone")
	bool bIsProne;

	UPROPERTY(EditAnywhere, Category = "Movement|Prone")
	float MaxWalkSpeedProne = 100.0f;

	// --- Interaction ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<AActor> TargetInteractable;

	// 화면을 즉시 까맣게 만듭니다 (로딩 렉을 가리기 위함)
	UFUNCTION(BlueprintCallable, Category = "Level Transition")
	void ForceBlackScreen();

	// 화면을 서서히 밝힙니다 (던전 생성 완료 후 호출)
	UFUNCTION(BlueprintCallable, Category = "Level Transition")
	void StartFadeIn(float FadeInDuration = 2.0f);

	// ==========================================
	// --- Currency System (재화 시스템) ---
	// ==========================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Currency")
	int32 CurrentGold = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Currency")
	int32 CurrentSapphire = 0;

	// 골드 획득, 소비, 초기화 (던전용)
	UFUNCTION(BlueprintCallable, Category = "Currency")
	void AddGold(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Currency")
	bool SpendGold(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Currency")
	void ResetGold();

	// 사파이어 획득, 소비 (영구 강화용)
	UFUNCTION(BlueprintCallable, Category = "Currency")
	void AddSapphire(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Currency")
	bool SpendSapphire(int32 Amount);

	// 테스트용 재화 치트키 (콘솔 창에서 실행 가능)
	UFUNCTION(Exec)
	void CheatCurrency(int32 GoldAmount, int32 SapphireAmount);

	// 플레이어의 인벤토리(가방) 컴포넌트입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UInventoryComponent> PlayerInventory;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	TArray<class APickupItemBase*> GetNearbyItems();

	// 현재 인벤토리가 열려있는지 확인하는 상태 변수
	bool bIsInventoryOpen = false;

	// 생성된 위젯을 담아둘 포인터
	UPROPERTY()
	TObjectPtr<class UUserWidget> InventoryWidgetInstance;

	// --- 시전 (Casting) 관련 ---
	UPROPERTY(BlueprintReadWrite, Category = "Casting")
	bool bIsCasting = false;

	// 시전 중 이동 속도 (앉아서 걷는 속도 수준)
	UPROPERTY(EditAnywhere, Category = "Casting")
	float CastingWalkSpeed = 160.0f;

	float OriginalWalkSpeed;

	// ItemID를 받도록 수정된 StartCasting
	void StartCasting(float Duration, FName ItemID, TFunction<void()> OnSuccess);

	// void StartCasting(float Duration, TFunction<void()> OnSuccess);
	void CancelCasting();

	// --- Buff & HOT System ---
	// 이동 속도 증가 적용 (중첩 방지)
	void ApplySpeedBuff(float BoostAmount, float Duration);

	// 지속 체력 회복 적용 (중첩 방지)
	void ApplyHealOverTime(float TotalHeal, float Duration);

	// 블루프린트에서 설정할 UI 클래스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<class UCastBarWidget> CastBarWidgetClass;

	UPROPERTY()
	UCastBarWidget* CastBarInstance = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void UpdateEquipmentVisuals(EEquipType EquipType, class UStaticMesh* NewMesh);

	// 아이템을 바닥에 버리는(스폰하는) 함수
	void DropItemToGround(FName ItemID);

	// 인벤토리 토글 함수
	void ToggleInventory();

	// [추가] 다음 스테이지로 넘어갔을 때 장비 외형을 다시 입혀주는 함수
	void RestoreEquipmentVisuals();

	void PerformFire(); // 실제로 총알이 나가는 로직
	void StopFiring();  // 마우스를 뗐을 때 연사를 멈추는 로직

	// 연사 속도를 조절할 타이머
	FTimerHandle AutoFireTimerHandle;

protected:
	// --- Lifecycle ---
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// --- Components ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> ADSCamera;

	// --- Input Actions ---
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> AimAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> ProneAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> CrouchAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> LeanAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> InteractAction;

	// --- Combat Settings ---
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Weapon")
	TSubclassOf<AWeaponBase> DefaultWeaponClass;

	UPROPERTY()
	TObjectPtr<AWeaponBase> CurrentWeapon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|State")
	bool bIsAiming = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|State")
	bool bIsFiring = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Spread")
	float CurrentSpread = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Combat|Spread")
	float SpreadPerShot = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Combat|Spread")
	float MaxSpread = 6.0f;

	UPROPERTY(EditAnywhere, Category = "Combat|Spread")
	float SpreadRecoveryRate = 5.0f;

	// --- Camera & ADS ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|ADS")
	FVector DynamicAimOffset;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|ADS")
	float ADSAlpha = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Combat|ADS")
	float AimDistance = 40.0f;

	// --- Animation & FX ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
	float MovementDirectionAngle = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
	float YawRotationSpeed = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
	float CurrentLean = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Animation|State")
	bool bIsChangingStance = false;

	UPROPERTY(EditAnywhere, Category = "Animation")
	TArray<TObjectPtr<UAnimMontage>> HitMontages;

	UPROPERTY(EditAnywhere, Category = "Animation|Interaction")
	TObjectPtr<UAnimMontage> InteractMontage;

	// ==========================================
	// --- UI & Inventory System ---
	// ==========================================

	// 에디터에서 할당할 인벤토리 위젯 클래스 (WBP_InventoryMain)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<class UUserWidget> InventoryWidgetClass;

	// 인벤토리 열기/닫기 단축키 (Tab 또는 I)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* InventoryAction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class USceneCaptureComponent2D> InventoryCamera;

	// --- 인벤토리 UI용 가짜(Clone) 몸통 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class USkeletalMeshComponent> InventoryCloneMesh;

	// UI에서 재생할 기본 대기(Idle) 애니메이션 에셋
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<class UAnimationAsset> UI_IdleAnimation;

	// --- 진짜 캐릭터 장비 메쉬 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Equipment")
	TObjectPtr<class UStaticMeshComponent> HelmetMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Equipment")
	TObjectPtr<class UStaticMeshComponent> VestMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Equipment")
	TObjectPtr<class UStaticMeshComponent> BackpackMesh;

	// --- 인벤토리 UI용 가짜 몸통 장비 메쉬 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Equipment")
	TObjectPtr<class UStaticMeshComponent> CloneHelmetMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Equipment")
	TObjectPtr<class UStaticMeshComponent> CloneVestMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Equipment")
	TObjectPtr<class UStaticMeshComponent> CloneBackpackMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Equipment")
	TObjectPtr<class UStaticMeshComponent> CloneWeaponMesh;

private:
	// --- Input Handlers ---
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartAiming();
	void StopAiming();
	void Fire();
	void StopFiringPose();
	void ToggleCrouch();
	void ToggleProne();
	void CustomJump();
	void InputLean(const FInputActionValue& Value);
	void InputLeanEnd(const FInputActionValue& Value);

	// --- Internal Logic ---
	void CheckForInteractables();
	void Interact();
	void Die();

	// --- Internal State ---
	FTimerHandle FireTimerHandle;
	FTimerHandle InteractionTimerHandle;

	float PreviousActorYaw = 0.0f;
	float LastFireTime = 0.0f;
	float TargetLean = 0.0f;
	float DefaultCapsuleHalfHeight;
	float DefaultMeshRelativeLocationZ;
	float DefaultFOV = 90.0f;
	float AimFOV = 60.0f;
	float ZoomInterpSpeed = 20.0f;
	float SpreadRecoveryDelay = 0.1f;
	bool bIsDead = false;

	FTimerHandle CastTimerHandle;
	TFunction<void()> OnCastSuccessCallback;

	// 이동 속도 버프 추적용
	bool bHasSpeedBuff = false;
	float CurrentSpeedBoost = 0.0f;
	FTimerHandle SpeedBuffTimerHandle;

	// 지속 회복(HOT) 추적용
	FTimerHandle HOTTimerHandle;
	int32 CurrentHOTTicks = 0;
	int32 MaxHOTTicks = 0;
	float HOTAmountPerTick = 0.0f;
};