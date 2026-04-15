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
	AGun_phiriaCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	// ==========================================
	// Getters
	// ==========================================
	FORCEINLINE TObjectPtr<USpringArmComponent> GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE TObjectPtr<UCameraComponent> GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE float GetCurrentSpread() const { return CurrentSpread; }
	FORCEINLINE TObjectPtr<AWeaponBase> GetCurrentWeapon() const { return CurrentWeapon; }

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool GetIsAiming() const { return bIsAiming; }

	// ==========================================
	// Stats & State
	// ==========================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MaxHealth = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float CurrentHealth;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats|State")
	bool bIsAimingAtHead = false;

	// ==========================================
	// Movement & Stance
	// ==========================================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	FVector LeanAxisCS;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Prone")
	bool bIsProne;

	UPROPERTY(EditAnywhere, Category = "Movement|Prone")
	float MaxWalkSpeedProne = 100.0f;

	// ==========================================
	// Interaction & Inventory
	// ==========================================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<AActor> TargetInteractable;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UInventoryComponent> PlayerInventory;

	bool bIsInventoryOpen = false;

	UPROPERTY()
	TObjectPtr<class UUserWidget> InventoryWidgetInstance;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	TArray<class APickupItemBase*> GetNearbyItems();

	UPROPERTY(EditDefaultsOnly, Category = "UI|Studio")
	TSubclassOf<class AInventoryStudio> StudioClass;

	UPROPERTY()
	TObjectPtr<class AInventoryStudio> SpawnedStudio;

	void DropItemToGround(FName ItemID);
	void ToggleInventory();
	void Reload();

	// ==========================================
	// Casting & Buffs
	// ==========================================
	UPROPERTY(BlueprintReadWrite, Category = "Casting")
	bool bIsCasting = false;

	UPROPERTY(EditAnywhere, Category = "Casting")
	float CastingWalkSpeed = 160.0f;

	void StartCasting(float Duration, FName ItemID, TFunction<void()> OnSuccess);
	void CancelCasting();
	void ApplySpeedBuff(float BoostAmount, float Duration);
	void ApplyHealOverTime(float TotalHeal, float Duration);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<class UCastBarWidget> CastBarWidgetClass;

	UPROPERTY()
	TObjectPtr <UCastBarWidget> CastBarInstance = nullptr;

	// ==========================================
	// Currency
	// ==========================================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Currency")
	int32 CurrentGold = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Currency")
	int32 CurrentSapphire = 0;

	UFUNCTION(BlueprintCallable, Category = "Currency") void AddGold(int32 Amount);
	UFUNCTION(BlueprintCallable, Category = "Currency") bool SpendGold(int32 Amount);
	UFUNCTION(BlueprintCallable, Category = "Currency") void ResetGold();
	UFUNCTION(BlueprintCallable, Category = "Currency") void AddSapphire(int32 Amount);
	UFUNCTION(BlueprintCallable, Category = "Currency") bool SpendSapphire(int32 Amount);

	UFUNCTION(Exec)
	void CheatCurrency(int32 GoldAmount, int32 SapphireAmount);

	// ==========================================
	// Equipment & Level Transition
	// ==========================================
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void UpdateEquipmentVisuals(EEquipType EquipType, class UStaticMesh* NewMesh);
	void RestoreEquipmentVisuals();

	UFUNCTION(BlueprintCallable, Category = "Level Transition") void ForceBlackScreen();
	UFUNCTION(BlueprintCallable, Category = "Level Transition") void StartFadeIn(float FadeInDuration = 2.0f);

	void EquipWeaponSlot(int32 SlotIndex);
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon") TArray<TObjectPtr<AWeaponBase>> WeaponSlots;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon") int32 ActiveWeaponSlot = 0;

	UFUNCTION(BlueprintCallable, Category = "Attachment")
	void AttachToHolster(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Studio")
	void RefreshStudioEquipment();

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// ==========================================
	// Components
	// ==========================================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components") TObjectPtr<USpringArmComponent> CameraBoom;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components") TObjectPtr<UCameraComponent> FollowCamera;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components") TObjectPtr<UCameraComponent> ADSCamera;

	// Mesh Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Equipment") TObjectPtr<class UStaticMeshComponent> HelmetMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Equipment") TObjectPtr<class UStaticMeshComponent> VestMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Equipment") TObjectPtr<class UStaticMeshComponent> BackpackMesh;

	// ==========================================
	// Inputs
	// ==========================================
	UPROPERTY(EditAnywhere, Category = "Input") TObjectPtr<UInputMappingContext> DefaultMappingContext;
	UPROPERTY(EditAnywhere, Category = "Input") TObjectPtr<UInputAction> JumpAction;
	UPROPERTY(EditAnywhere, Category = "Input") TObjectPtr<UInputAction> MoveAction;
	UPROPERTY(EditAnywhere, Category = "Input") TObjectPtr<UInputAction> LookAction;
	UPROPERTY(EditAnywhere, Category = "Input") TObjectPtr<UInputAction> AimAction;
	UPROPERTY(EditAnywhere, Category = "Input") TObjectPtr<UInputAction> FireAction;
	UPROPERTY(EditAnywhere, Category = "Input") TObjectPtr<UInputAction> ProneAction;
	UPROPERTY(EditAnywhere, Category = "Input") TObjectPtr<UInputAction> CrouchAction;
	UPROPERTY(EditAnywhere, Category = "Input") TObjectPtr<UInputAction> LeanAction;
	UPROPERTY(EditAnywhere, Category = "Input") TObjectPtr<UInputAction> InteractAction;
	UPROPERTY(EditAnywhere, Category = "Input") TObjectPtr<UInputAction> InventoryAction;
	UPROPERTY(EditAnywhere, Category = "Input") TObjectPtr<UInputAction> EquipWeapon1Action;
	UPROPERTY(EditAnywhere, Category = "Input") TObjectPtr<UInputAction> EquipWeapon2Action;
	UPROPERTY(EditAnywhere, Category = "Input") TObjectPtr<UInputAction> EquipWeapon3Action;
	UPROPERTY(EditAnywhere, Category = "Input") TObjectPtr<UInputAction> EquipWeapon4Action;

	// ==========================================
	// Combat & Animation Settings
	// ==========================================
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Weapon") TSubclassOf<AWeaponBase> DefaultWeaponClass;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon") TObjectPtr<AWeaponBase> CurrentWeapon;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|State") bool bIsAiming = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|State") bool bIsFiring = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Spread") float CurrentSpread = 0.0f;
	UPROPERTY(EditAnywhere, Category = "Combat|Spread") float SpreadPerShot = 1.0f;
	UPROPERTY(EditAnywhere, Category = "Combat|Spread") float MaxSpread = 6.0f;
	UPROPERTY(EditAnywhere, Category = "Combat|Spread") float SpreadRecoveryRate = 5.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|ADS") FVector DynamicAimOffset;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|ADS") float ADSAlpha = 0.0f;
	UPROPERTY(EditAnywhere, Category = "Combat|ADS") float AimDistance = 40.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation") float MovementDirectionAngle = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation") float YawRotationSpeed = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation") float CurrentLean = 0.0f;
	UPROPERTY(BlueprintReadWrite, Category = "Animation|State") bool bIsChangingStance = false;

	UPROPERTY(EditAnywhere, Category = "Animation") TArray<TObjectPtr<UAnimMontage>> HitMontages;
	UPROPERTY(EditAnywhere, Category = "Animation|Interaction") TObjectPtr<UAnimMontage> InteractMontage;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI") TObjectPtr<class UAnimationAsset> UI_IdleAnimation;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI") TSubclassOf<class UUserWidget> InventoryWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Obstruction") float WeaponObstructionAlpha = 0.0f;

private:
	// --- Input Handlers ---
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartAiming();
	void StopAiming();
	void Fire();
	void StopFiring();
	void PerformFire();
	void StopFiringPose();
	void ToggleCrouch();
	void ToggleProne();
	void CustomJump();
	void InputLean(const FInputActionValue& Value);
	void InputLeanEnd(const FInputActionValue& Value);
	void Interact();

	void EquipWeapon1();
	void EquipWeapon2();
	void EquipWeapon3();
	void EquipWeapon4();

	// --- Core Logic Refactored Helpers ---
	void InitializeWeapon();
	void InitializeInventoryUI();
	void CheckForInteractables();
	void Die();
	float ProcessArmorDurability(float Damage, bool bIsHeadshot);

	// Tick 도우미 함수들
	void UpdateCameraFOV(float DeltaTime);
	void UpdateWeaponSpread(float DeltaTime);
	void UpdateADSOffset(float DeltaTime);
	void UpdateLocomotionVariables(float DeltaTime);
	void CheckHeadshotTarget();
	void UpdateLeanAndCameraOffset(float DeltaTime);
	void PreventWeaponClipping();

	// --- Internal State ---
	FTimerHandle FireTimerHandle;
	FTimerHandle AutoFireTimerHandle;
	FTimerHandle InteractionTimerHandle;
	FTimerHandle CastTimerHandle;
	FTimerHandle SpeedBuffTimerHandle;
	FTimerHandle HOTTimerHandle;

	TFunction<void()> OnCastSuccessCallback;

	float PreviousActorYaw = 0.0f;
	float LastFireTime = 0.0f;
	float TargetLean = 0.0f;
	float DefaultCapsuleHalfHeight = 96.0f;
	float DefaultMeshRelativeLocationZ = -97.0f;
	float DefaultFOV = 90.0f;
	float AimFOV = 60.0f;
	float ZoomInterpSpeed = 20.0f;
	float SpreadRecoveryDelay = 0.1f;
	float OriginalWalkSpeed = 0.0f;
	bool bIsDead = false;

	// Buff states
	bool bHasSpeedBuff = false;
	float CurrentSpeedBoost = 0.0f;
	int32 CurrentHOTTicks = 0;
	int32 MaxHOTTicks = 0;
	float HOTAmountPerTick = 0.0f;
};