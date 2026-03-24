#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Weapon/WeaponBase.h"
#include "Gun_phiriaCharacter.generated.h"

// 전방 선언
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UStaticMeshComponent;
class UNiagaraSystem;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config = Game)
class GUN_PHIRIA_API AGun_phiriaCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AGun_phiriaCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	/* Getters */
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE float GetCurrentSpread() const { return CurrentSpread; }
	FORCEINLINE class AWeaponBase* GetCurrentWeapon() const { return CurrentWeapon; }

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool GetIsAiming() const { return bIsAiming; }

	/* Public Stats & States (HUD 연동용) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crosshair")
	bool bIsAimingAtHead = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MaxHealth = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float CurrentHealth;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	FVector LeanAxisCS;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Prone")
	bool bIsProne;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Prone")
	float MaxWalkSpeedProne = 100.0f;

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/* Components */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> ADSCamera;

	/* Input Actions */
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> CrouchAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> LeanAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> InteractAction;

	/* Input Handlers */
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

	/* Weapon & Combat */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<AWeaponBase> DefaultWeaponClass;

	UPROPERTY()
	AWeaponBase* CurrentWeapon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	bool bIsAiming = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	bool bIsFiring = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	float CurrentSpread = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float SpreadPerShot = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float MaxSpread = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float SpreadRecoveryRate = 5.0f;

	float SpreadRecoveryDelay = 0.1f;

	/* Aiming & Camera Settings */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aiming")
	FVector DynamicAimOffset;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aiming")
	float ADSAlpha = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aiming")
	float AimDistance = 40.0f;

	float DefaultFOV = 90.0f;
	float AimFOV = 60.0f;
	float ZoomInterpSpeed = 20.0f;

	/* Animation & Locomotion */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
	float MovementDirectionAngle = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
	float YawRotationSpeed = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
	float CurrentLean = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "State")
	bool bIsChangingStance = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TArray<class UAnimMontage*> HitMontages;

	float TargetLean = 0.0f;
	float DefaultCapsuleHalfHeight;
	float DefaultMeshRelativeLocationZ;

	FTimerHandle InteractionTimerHandle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	AActor* TargetInteractable;

	void CheckForInteractables();

	void Interact();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	class UAnimMontage* InteractMontage;

private:
	/* Internal State */
	FTimerHandle FireTimerHandle;
	float PreviousActorYaw = 0.0f;
	float LastFireTime = 0.0f;
	bool bIsDead = false;

	void Die();
};