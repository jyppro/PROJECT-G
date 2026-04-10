#include "Gun_phiriaCharacter.h"
#include "Weapon/WeaponBase.h"
#include "Interface/InteractInterface.h"
#include "component/InventoryComponent.h"
#include "Blueprint/UserWidget.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Item/PickupItemBase.h"
#include "UI/InventoryMainWidget.h"
#include "UI/CastBarWidget.h"
#include "Gun_phiriaGameInstance.h"
#include "UI/InventoryStudio.h"

#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/DamageEvents.h"
#include "Engine/OverlapResult.h"
#include "TimerManager.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "Components/SceneComponent.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

AGun_phiriaCharacter::AGun_phiriaCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Collision & Physics
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	if (TObjectPtr<UCharacterMovementComponent> MoveComp = GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = false;
		MoveComp->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
		MoveComp->JumpZVelocity = 700.f;
		MoveComp->AirControl = 0.35f;
		MoveComp->MaxWalkSpeed = 500.f;
		MoveComp->NavAgentProps.bCanCrouch = true;
		MoveComp->MaxWalkSpeedCrouched = 250.0f;
	}

	// Camera Setup
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

	ADSCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ADSCamera"));
	ADSCamera->SetupAttachment(GetMesh());
	ADSCamera->SetAutoActivate(false);
	ADSCamera->PrimaryComponentTick.TickGroup = TG_PostPhysics;

	PlayerInventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("PlayerInventory"));

	HelmetMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HelmetMesh"));
	HelmetMesh->SetupAttachment(GetMesh(), FName("HelmetSocket"));
	VestMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VestMesh"));
	VestMesh->SetupAttachment(GetMesh(), FName("VestSocket"));
	BackpackMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BackpackMesh"));
	BackpackMesh->SetupAttachment(GetMesh(), FName("BackpackSocket"));
}

void AGun_phiriaCharacter::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;

	// 1. Data Load
	if (UGun_phiriaGameInstance* GameInst = Cast<UGun_phiriaGameInstance>(GetGameInstance()))
	{
		GameInst->LoadPlayerData(this);
		RestoreEquipmentVisuals();
	}

	// 2. Timers & Logic Init
	GetWorldTimerManager().SetTimer(InteractionTimerHandle, this, &AGun_phiriaCharacter::CheckForInteractables, 0.1f, true);
	if (ADSCamera) ADSCamera->AddTickPrerequisiteComponent(GetMesh());

	// 3. Components Init (Refactored)
	InitializeWeapon();
	InitializeInventoryUI();

	if (StudioClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		// 안 보이는 맵 저 아래에 스폰합니다.
		FVector StudioLocation = FVector(0.f, 0.f, -10000.f);
		SpawnedStudio = GetWorld()->SpawnActor<AInventoryStudio>(StudioClass, StudioLocation, FRotator::ZeroRotator, SpawnParams);
	}
}

// ==========================================
// Tick & Tick Helpers (Refactored)
// ==========================================
void AGun_phiriaCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateCameraFOV(DeltaTime);
	UpdateWeaponSpread(DeltaTime);
	UpdateADSOffset(DeltaTime);
	UpdateLocomotionVariables(DeltaTime);
	CheckHeadshotTarget();
	UpdateLeanAndCameraOffset(DeltaTime);
	PreventWeaponClipping();
}

void AGun_phiriaCharacter::UpdateCameraFOV(float DeltaTime)
{
	TObjectPtr<UCameraComponent> ActiveCam = bIsAiming ? ADSCamera : FollowCamera;
	float TargetFOV = bIsAiming ? AimFOV : DefaultFOV;
	ActiveCam->SetFieldOfView(FMath::FInterpTo(ActiveCam->FieldOfView, TargetFOV, DeltaTime, ZoomInterpSpeed));
}

void AGun_phiriaCharacter::UpdateWeaponSpread(float DeltaTime)
{
	float TargetMinSpread = 1.0f;
	const float Speed = GetVelocity().Size2D();
	const bool bFalling = GetCharacterMovement()->IsFalling();

	if (bFalling) TargetMinSpread = bIsAiming ? 3.0f : 6.0f;
	else if (Speed > 10.0f)
	{
		if (bIsProne) TargetMinSpread = bIsAiming ? 0.5f : 1.5f;
		else if (bIsCrouched) TargetMinSpread = bIsAiming ? 1.0f : 2.0f;
		else TargetMinSpread = bIsAiming ? 1.5f : 3.0f;
	}
	else
	{
		if (bIsProne) TargetMinSpread = bIsAiming ? 0.0f : 0.2f;
		else if (bIsCrouched) TargetMinSpread = bIsAiming ? 0.0f : 0.5f;
		else TargetMinSpread = bIsAiming ? 0.0f : 1.0f;
	}

	if (GetWorld()->GetTimeSeconds() - LastFireTime > SpreadRecoveryDelay)
	{
		CurrentSpread = FMath::FInterpTo(CurrentSpread, TargetMinSpread, DeltaTime, SpreadRecoveryRate);
	}
}

void AGun_phiriaCharacter::UpdateADSOffset(float DeltaTime)
{
	ADSAlpha = FMath::FInterpTo(ADSAlpha, bIsAiming ? 1.0f : 0.0f, DeltaTime, 15.0f);
	if (ADSAlpha > 0.01f && FollowCamera && CurrentWeapon && CurrentWeapon->GetWeaponMesh())
	{
		const FVector CamLoc = FollowCamera->GetComponentLocation();
		const FVector CamFwd = FollowCamera->GetForwardVector();
		const float TotalDist = FVector::DotProduct(GetActorLocation() - CamLoc, CamFwd) + AimDistance;

		const FVector TargetSightLoc = CamLoc + (CamFwd * TotalDist);
		const FVector HandLoc = CurrentWeapon->GetWeaponMesh()->GetComponentLocation();
		const FVector SightLoc = CurrentWeapon->GetWeaponMesh()->GetSocketLocation(FName("SightSocket"));

		const FVector TargetHandLoc = TargetSightLoc + (HandLoc - SightLoc);
		DynamicAimOffset = FMath::Lerp(FVector::ZeroVector, GetMesh()->GetComponentTransform().InverseTransformPosition(TargetHandLoc), ADSAlpha);
	}
	else DynamicAimOffset = FVector::ZeroVector;
}

void AGun_phiriaCharacter::UpdateLocomotionVariables(float DeltaTime)
{
	const FVector Velocity = GetVelocity();
	MovementDirectionAngle = (Velocity.Size2D() > 1.0f) ? GetActorTransform().InverseTransformVectorNoScale(Velocity).Rotation().Yaw : 0.0f;

	const float CurrentYaw = GetActorRotation().Yaw;
	float YawDelta = FRotator::NormalizeAxis(CurrentYaw - PreviousActorYaw);
	YawRotationSpeed = YawDelta / DeltaTime;
	PreviousActorYaw = CurrentYaw;
}

void AGun_phiriaCharacter::CheckHeadshotTarget()
{
	bIsAimingAtHead = false;
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		FVector CamLoc; FRotator CamRot;
		PC->GetPlayerViewPoint(CamLoc, CamRot);
		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);
		if (CurrentWeapon) Params.AddIgnoredActor(CurrentWeapon);

		if (GetWorld()->LineTraceSingleByChannel(Hit, CamLoc, CamLoc + CamRot.Vector() * 5000.f, ECC_Visibility, Params))
		{
			if (Cast<ACharacter>(Hit.GetActor()) && Hit.BoneName == FName("head")) bIsAimingAtHead = true;
		}
	}
}

void AGun_phiriaCharacter::UpdateLeanAndCameraOffset(float DeltaTime)
{
	CurrentLean = FMath::FInterpTo(CurrentLean, TargetLean, DeltaTime, 10.0f);
	LeanAxisCS = GetMesh()->GetComponentTransform().InverseTransformVectorNoScale(GetBaseAimRotation().Vector());

	if (CameraBoom)
	{
		float H = bIsProne ? 60.f : (bIsCrouched ? 90.f : 120.f);
		CameraBoom->SocketOffset = FMath::VInterpTo(CameraBoom->SocketOffset, FVector(0.f, CurrentLean * 100.f, H), DeltaTime, 10.0f);
	}
}

void AGun_phiriaCharacter::PreventWeaponClipping()
{
	if (CurrentWeapon && CurrentWeapon->GetWeaponMesh())
	{
		const FVector Muzzle = CurrentWeapon->GetWeaponMesh()->GetSocketLocation(FName("MuzzleSocket"));
		FVector Start = GetActorLocation(); Start.Z = Muzzle.Z;
		FVector End = Muzzle + (Muzzle - Start).GetSafeNormal() * 15.f;

		FHitResult Hit;
		FCollisionQueryParams Params; Params.AddIgnoredActor(this); Params.AddIgnoredActor(CurrentWeapon);

		if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
		{
			if (FMath::Abs(Hit.ImpactNormal.Z) < 0.3f)
			{
				FVector Push = Hit.ImpactNormal; Push.Z = 0.f;
				AddActorWorldOffset(Push.GetSafeNormal() * FVector::Distance(Hit.ImpactPoint, End), true);
			}
		}
	}
}

// ==========================================
// Setup & Initialization Helpers (Refactored)
// ==========================================
void AGun_phiriaCharacter::InitializeWeapon()
{
	if (!DefaultWeaponClass) return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	CurrentWeapon = GetWorld()->SpawnActor<AWeaponBase>(DefaultWeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

	if (CurrentWeapon && CurrentWeapon->GetWeaponMesh())
	{
		FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepRelative, true);
		CurrentWeapon->AttachToComponent(GetMesh(), AttachRules, FName("WeaponSocket"));

		FTransform GripSocketRelative = CurrentWeapon->GetWeaponMesh()->GetSocketTransform(FName("RightHandGripSocket"), ERelativeTransformSpace::RTS_Actor);
		GripSocketRelative.SetScale3D(FVector(1.0f, 1.0f, 1.0f));
		FTransform InverseGrip = GripSocketRelative.Inverse();

		CurrentWeapon->SetActorRelativeLocation(InverseGrip.GetLocation());
		CurrentWeapon->SetActorRelativeRotation(InverseGrip.GetRotation());

		ADSCamera->AttachToComponent(CurrentWeapon->GetWeaponMesh(), AttachRules, FName("SightSocket"));
	}
}

void AGun_phiriaCharacter::InitializeInventoryUI()
{
	if (InventoryWidgetClass)
	{
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			InventoryWidgetInstance = CreateWidget<UUserWidget>(PC, InventoryWidgetClass);
			if (InventoryWidgetInstance)
			{
				InventoryWidgetInstance->AddToViewport();
				InventoryWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}
}

// ==========================================
// Input Bindings & Movement
// ==========================================
void AGun_phiriaCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	if (TObjectPtr<UEnhancedInputComponent> EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &AGun_phiriaCharacter::CustomJump);
		EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AGun_phiriaCharacter::Move);
		EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &AGun_phiriaCharacter::Look);
		EIC->BindAction(AimAction, ETriggerEvent::Started, this, &AGun_phiriaCharacter::StartAiming);
		EIC->BindAction(AimAction, ETriggerEvent::Completed, this, &AGun_phiriaCharacter::StopAiming);
		EIC->BindAction(FireAction, ETriggerEvent::Started, this, &AGun_phiriaCharacter::Fire);
		EIC->BindAction(FireAction, ETriggerEvent::Completed, this, &AGun_phiriaCharacter::StopFiring);
		EIC->BindAction(CrouchAction, ETriggerEvent::Started, this, &AGun_phiriaCharacter::ToggleCrouch);
		EIC->BindAction(LeanAction, ETriggerEvent::Triggered, this, &AGun_phiriaCharacter::InputLean);
		EIC->BindAction(LeanAction, ETriggerEvent::Completed, this, &AGun_phiriaCharacter::InputLeanEnd);
		EIC->BindAction(ProneAction, ETriggerEvent::Started, this, &AGun_phiriaCharacter::ToggleProne);
		EIC->BindAction(InteractAction, ETriggerEvent::Started, this, &AGun_phiriaCharacter::Interact);
		EIC->BindAction(InventoryAction, ETriggerEvent::Started, this, &AGun_phiriaCharacter::ToggleInventory);
	}
}

void AGun_phiriaCharacter::Move(const FInputActionValue& Value)
{
	if (!Controller) return;

	const FVector2D Vec = Value.Get<FVector2D>();
	const FRotator Rot = FRotator(0, Controller->GetControlRotation().Yaw, 0);
	const FVector Fwd = FRotationMatrix(Rot).GetUnitAxis(EAxis::X);
	const FVector Rit = FRotationMatrix(Rot).GetUnitAxis(EAxis::Y);

	float FwdInput = Vec.Y;
	if (CurrentWeapon && CurrentWeapon->GetWeaponMesh() && FwdInput > 0.0f)
	{
		const FVector Muzzle = CurrentWeapon->GetWeaponMesh()->GetSocketLocation(FName("MuzzleSocket"));
		FHitResult Hit;
		FCollisionQueryParams Params; Params.AddIgnoredActor(this); Params.AddIgnoredActor(CurrentWeapon);
		if (GetWorld()->SweepSingleByChannel(Hit, Muzzle - Fwd * 20.f, Muzzle + Fwd * 15.f, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(10.f), Params))
		{
			FwdInput = 0.0f;
		}
	}

	AddMovementInput(Fwd, FwdInput);
	AddMovementInput(Rit, Vec.X);
}

void AGun_phiriaCharacter::Look(const FInputActionValue& Value)
{
	if (bIsInventoryOpen) return;
	const FVector2D Vec = Value.Get<FVector2D>();
	AddControllerYawInput(Vec.X);
	AddControllerPitchInput(Vec.Y);
}

void AGun_phiriaCharacter::CustomJump()
{
	if (bIsCasting) { CancelCasting(); return; }
	if (bIsChangingStance) return;

	if (bIsProne) ToggleProne();
	else if (bIsCrouched) UnCrouch();
	else Jump();
}

void AGun_phiriaCharacter::ToggleCrouch()
{
	if (bIsCasting) { CancelCasting(); return; }
	if (!bIsProne) bIsCrouched ? UnCrouch() : Crouch();
}

void AGun_phiriaCharacter::ToggleProne()
{
	if (bIsCasting) { CancelCasting(); return; }
	if (GetCharacterMovement()->IsFalling()) return;

	if (bIsProne)
	{
		bIsProne = false;
		GetCapsuleComponent()->SetCapsuleHalfHeight(DefaultCapsuleHalfHeight);
		GetMesh()->SetRelativeLocation(FVector(0, 0, DefaultMeshRelativeLocationZ));
		GetCharacterMovement()->MaxWalkSpeed = 500.0f;
	}
	else
	{
		if (bIsCrouched) UnCrouch();
		bIsProne = true;
		GetCapsuleComponent()->SetCapsuleHalfHeight(34.0f);
		GetMesh()->SetRelativeLocation(FVector(0, 0, DefaultMeshRelativeLocationZ + (DefaultCapsuleHalfHeight - 34.0f)));
		GetCharacterMovement()->MaxWalkSpeed = MaxWalkSpeedProne;
	}
}

void AGun_phiriaCharacter::InputLean(const FInputActionValue& Value) { TargetLean = Value.Get<float>(); }
void AGun_phiriaCharacter::InputLeanEnd(const FInputActionValue& Value) { TargetLean = 0.0f; }

// ==========================================
// Combat
// ==========================================
void AGun_phiriaCharacter::StartAiming()
{
	if (bIsInventoryOpen) return;
	if (bIsCasting) { CancelCasting(); return; }

	bIsAiming = true;
	if (FollowCamera) FollowCamera->SetActive(false);
	if (ADSCamera) ADSCamera->SetActive(true);
}

void AGun_phiriaCharacter::StopAiming()
{
	bIsAiming = false;
	if (ADSCamera) ADSCamera->SetActive(false);
	if (FollowCamera) FollowCamera->SetActive(true);
}

void AGun_phiriaCharacter::Fire()
{
	if (bIsInventoryOpen || bIsCasting) { CancelCasting(); return; }
	if (!CurrentWeapon || !FollowCamera || !ADSCamera) return;

	if (CurrentWeapon->FireMode == EFireMode::Auto)
	{
		float TimeBetweenShots = 60.0f / CurrentWeapon->FireRate;
		GetWorldTimerManager().SetTimer(AutoFireTimerHandle, this, &AGun_phiriaCharacter::PerformFire, TimeBetweenShots, true, 0.0f);
	}
	else
	{
		PerformFire();
	}
}

void AGun_phiriaCharacter::StopFiring()
{
	GetWorldTimerManager().ClearTimer(AutoFireTimerHandle);
}

void AGun_phiriaCharacter::StopFiringPose() { bIsFiring = false; }

void AGun_phiriaCharacter::PerformFire()
{
	if (!CurrentWeapon) return;

	if (!CurrentWeapon->bInfiniteAmmo && CurrentWeapon->CurrentAmmo <= 0)
	{
		StopFiring();
		CurrentWeapon->Fire(FVector::ZeroVector);
		return;
	}

	bIsFiring = true;
	GetWorldTimerManager().SetTimer(FireTimerHandle, this, &AGun_phiriaCharacter::StopFiringPose, 1.0f, false);
	LastFireTime = GetWorld()->GetTimeSeconds();

	if (TObjectPtr<UAnimInstance> AnimInst = GetMesh()->GetAnimInstance())
	{
		const auto& Montages = bIsProne ? CurrentWeapon->ProneFireMontages : CurrentWeapon->StandCrouchFireMontages;
		if (Montages.Num() > 0) AnimInst->Montage_Play(Montages[FMath::RandRange(0, Montages.Num() - 1)]);
	}

	float Mult = (bIsAiming ? 0.6f : 1.2f) * (GetVelocity().Size2D() > 10.f ? 1.5f : 1.f) * (GetCharacterMovement()->IsFalling() ? 2.5f : 1.f);
	Mult *= bIsProne ? 0.4f : (bIsCrouched ? 0.75f : 1.f);

	AddControllerPitchInput(FMath::RandRange(-0.5f, -1.0f) * Mult);
	AddControllerYawInput(FMath::RandRange(-0.5f, 0.5f) * Mult);
	CurrentSpread = FMath::Clamp(CurrentSpread + (SpreadPerShot * Mult), 0.0f, MaxSpread);

	TObjectPtr<UCameraComponent> ActiveCam = bIsAiming ? ADSCamera : FollowCamera;
	const FVector CamLoc = ActiveCam->GetComponentLocation();
	const FVector CamEnd = CamLoc + ActiveCam->GetForwardVector() * 10000.f;

	FHitResult CamHit;
	FCollisionQueryParams Params; Params.AddIgnoredActor(this); Params.AddIgnoredActor(CurrentWeapon);
	const FVector EyeTarget = GetWorld()->LineTraceSingleByChannel(CamHit, CamLoc, CamEnd, ECC_Visibility, Params) ? CamHit.ImpactPoint : CamEnd;

	const FVector Muzzle = CurrentWeapon->GetWeaponMesh()->GetSocketLocation(FName("MuzzleSocket"));
	const FVector RealDir = (EyeTarget - Muzzle).GetSafeNormal();
	FHitResult FinalHit;
	const FVector FinalTarget = GetWorld()->LineTraceSingleByChannel(FinalHit, Muzzle, Muzzle + RealDir * 5000.f, ECC_Visibility, Params) ? FinalHit.ImpactPoint : Muzzle + RealDir * 5000.f;

	FVector Right, Up; RealDir.FindBestAxisVectors(Right, Up);
	const float Radius = FMath::Tan(FMath::DegreesToRadians(CurrentSpread * CurrentWeapon->WeaponSpreadMultiplier)) * FVector::Distance(Muzzle, FinalTarget);
	const float RandAngle = FMath::RandRange(0.f, PI * 2.f);
	const FVector Offset = (Right * FMath::Cos(RandAngle) + Up * FMath::Sin(RandAngle)) * FMath::RandRange(0.f, Radius);

	CurrentWeapon->Fire(FinalTarget + Offset);
}

// ==========================================
// Health & Damage (Refactored)
// ==========================================
float AGun_phiriaCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead) return 0.0f;

	float ActualDamage = DamageAmount;
	bool bIsHeadshot = false;

	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		FName HitBoneName = static_cast<const FPointDamageEvent*>(&DamageEvent)->HitInfo.BoneName;
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::White, FString::Printf(TEXT("Hit Part: %s / Damage: %.1f"), *HitBoneName.ToString(), ActualDamage));

		if (HitBoneName == FName("head"))
		{
			ActualDamage *= 2.5f;
			bIsHeadshot = true;
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, FString::Printf(TEXT("!!! HeadShot !!! Powerful Damage: %.1f"), ActualDamage));
		}
	}

	// 방어구 데미지 계산 분리
	ActualDamage = ProcessArmorDurability(ActualDamage, bIsHeadshot);

	ActualDamage = Super::TakeDamage(ActualDamage, DamageEvent, EventInstigator, DamageCauser);
	if (HitMontages.Num() > 0) PlayAnimMontage(HitMontages[FMath::RandRange(0, HitMontages.Num() - 1)]);

	CurrentHealth = FMath::Clamp(CurrentHealth - ActualDamage, 0.0f, MaxHealth);
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, FString::Printf(TEXT("-> Player Final Attack Damage: %.1f / Remain Health: %.1f"), ActualDamage, CurrentHealth));

	if (CurrentHealth <= 0.0f) Die();

	return ActualDamage;
}

float AGun_phiriaCharacter::ProcessArmorDurability(float Damage, bool bIsHeadshot)
{
	if (!PlayerInventory) return Damage;

	float OriginalDamage = Damage;
	float ResultDamage = Damage;

	if (bIsHeadshot && !PlayerInventory->EquippedHelmetID.IsNone())
	{
		if (FItemData* HelmetData = PlayerInventory->ItemDataTable->FindRow<FItemData>(PlayerInventory->EquippedHelmetID, TEXT("HelmetDefense")))
		{
			float BlockedDamage = OriginalDamage * HelmetData->DefensePower;
			ResultDamage -= BlockedDamage;
			PlayerInventory->CurrentHelmetDurability -= OriginalDamage;

			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Cyan, FString::Printf(TEXT("[Helmet Defense] %.1f Damage Blocked! / Remaining Durability: %.1f"), BlockedDamage, PlayerInventory->CurrentHelmetDurability));

			if (PlayerInventory->CurrentHelmetDurability <= 0.0f)
			{
				PlayerInventory->MaxWeight -= HelmetData->StatBonus;
				PlayerInventory->EquippedHelmetID = NAME_None;
				PlayerInventory->CurrentHelmetDurability = 0.0f;
				UpdateEquipmentVisuals(EEquipType::Helmet, nullptr);

				if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, TEXT("!!! Helmet Destroyed !!!"));
				if (bIsInventoryOpen && InventoryWidgetInstance)
				{
					if (UInventoryMainWidget* MainWidget = Cast<UInventoryMainWidget>(InventoryWidgetInstance)) MainWidget->UpdateEquipmentUI();
				}
			}
		}
	}
	else if (!bIsHeadshot && !PlayerInventory->EquippedVestID.IsNone())
	{
		if (FItemData* VestData = PlayerInventory->ItemDataTable->FindRow<FItemData>(PlayerInventory->EquippedVestID, TEXT("VestDefense")))
		{
			float BlockedDamage = OriginalDamage * VestData->DefensePower;
			ResultDamage -= BlockedDamage;
			PlayerInventory->CurrentVestDurability -= OriginalDamage;

			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Cyan, FString::Printf(TEXT("[Vest Defense] %.1f Damage Blocked! / Remaining Durability: %.1f"), BlockedDamage, PlayerInventory->CurrentVestDurability));

			if (PlayerInventory->CurrentVestDurability <= 0.0f)
			{
				PlayerInventory->MaxWeight -= VestData->StatBonus;
				PlayerInventory->EquippedVestID = NAME_None;
				PlayerInventory->CurrentVestDurability = 0.0f;
				UpdateEquipmentVisuals(EEquipType::Vest, nullptr);

				if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, TEXT("!!! Vest Destroyed !!!"));
				if (bIsInventoryOpen && InventoryWidgetInstance)
				{
					if (UInventoryMainWidget* MainWidget = Cast<UInventoryMainWidget>(InventoryWidgetInstance)) MainWidget->UpdateEquipmentUI();
				}
			}
		}
	}

	return ResultDamage;
}

void AGun_phiriaCharacter::Die()
{
	if (bIsDead) return;
	bIsDead = true;
	DisableInput(Cast<APlayerController>(GetController()));
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	GetMesh()->SetSimulatePhysics(true);
}

// ==========================================
// Interaction & Inventory
// ==========================================
void AGun_phiriaCharacter::CheckForInteractables()
{
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params; Params.AddIgnoredActor(this);
	TObjectPtr<AActor> BestTarget = nullptr;

	if (GetWorld()->OverlapMultiByChannel(Overlaps, GetActorLocation(), FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(200.f), Params))
	{
		float MinDist = MAX_FLT;
		for (const auto& Res : Overlaps)
		{
			if (Res.GetActor() && Res.GetActor()->Implements<UInteractInterface>())
			{
				float D = FVector::Dist(GetActorLocation(), Res.GetActor()->GetActorLocation());
				if (D < MinDist) { MinDist = D; BestTarget = Res.GetActor(); }
			}
		}
	}
	if (TargetInteractable != BestTarget) TargetInteractable = BestTarget;
}

void AGun_phiriaCharacter::Interact()
{
	if (TargetInteractable && TargetInteractable->Implements<UInteractInterface>())
	{
		IInteractInterface::Execute_Interact(TargetInteractable.Get(), this);

		if (bIsInventoryOpen && InventoryWidgetInstance)
		{
			UFunction* RefreshFunc = InventoryWidgetInstance->FindFunction(FName("RefreshInventory"));
			if (RefreshFunc) InventoryWidgetInstance->ProcessEvent(RefreshFunc, nullptr);
		}
	}
}

void AGun_phiriaCharacter::ToggleInventory()
{
	if (!InventoryWidgetInstance) return;
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	bIsInventoryOpen = !bIsInventoryOpen;

	if (bIsInventoryOpen)
	{
		if (SpawnedStudio)
		{
			SpawnedStudio->CaptureProfile();
		}

		InventoryWidgetInstance->SetVisibility(ESlateVisibility::Visible);

		if (SpawnedStudio)
		{
			SpawnedStudio->SetCameraLive(true);
		}

		UFunction* RefreshFunc = InventoryWidgetInstance->FindFunction(FName("RefreshInventory"));
		if (RefreshFunc) InventoryWidgetInstance->ProcessEvent(RefreshFunc, nullptr);

		if (UInventoryMainWidget* MainWidget = Cast<UInventoryMainWidget>(InventoryWidgetInstance))
		{
			MainWidget->CurrentMode = EInventoryMode::IM_Normal;
			MainWidget->StartNearbyTimer();
			MainWidget->ForceNearbyRefresh();
		}

		PC->SetShowMouseCursor(true);
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(InventoryWidgetInstance->TakeWidget());
		PC->SetInputMode(InputMode);
	}
	else
	{
		InventoryWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);

		if (SpawnedStudio)
		{
			SpawnedStudio->SetCameraLive(false);
		}

		if (UInventoryMainWidget* MainWidget = Cast<UInventoryMainWidget>(InventoryWidgetInstance)) MainWidget->StopNearbyTimer();

		PC->SetShowMouseCursor(false);
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
	}
}

TArray<APickupItemBase*> AGun_phiriaCharacter::GetNearbyItems()
{
	TArray<APickupItemBase*> NearbyItems;
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params; Params.AddIgnoredActor(this);

	if (GetWorld()->OverlapMultiByChannel(Overlaps, GetActorLocation(), FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(200.f), Params))
	{
		for (const auto& Res : Overlaps)
		{
			if (APickupItemBase* FoundItem = Cast<APickupItemBase>(Res.GetActor())) NearbyItems.AddUnique(FoundItem);
		}
	}
	return NearbyItems;
}

void AGun_phiriaCharacter::DropItemToGround(FName ItemID)
{
	if (!PlayerInventory || !PlayerInventory->ItemDataTable) return;

	bool bRemoved = PlayerInventory->RemoveItem(ItemID, 1);
	if (!bRemoved) return;

	FItemData* ItemData = PlayerInventory->ItemDataTable->FindRow<FItemData>(ItemID, TEXT("DropItem"));
	if (ItemData && ItemData->ItemClass)
	{
		FVector SpawnLoc = GetActorLocation() + (GetActorForwardVector() * 100.0f);
		SpawnLoc.Z -= 80.0f;

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		APickupItemBase* DroppedItem = GetWorld()->SpawnActor<APickupItemBase>(ItemData->ItemClass, SpawnLoc, FRotator::ZeroRotator, SpawnParams);

		if (DroppedItem)
		{
			DroppedItem->ItemID = ItemID;
			DroppedItem->Quantity = 1;
		}
	}
}

// ==========================================
// Casting & Buffs
// ==========================================
void AGun_phiriaCharacter::StartCasting(float Duration, FName ItemID, TFunction<void()> OnSuccess)
{
	if (bIsCasting) CancelCasting();

	bIsCasting = true;
	OnCastSuccessCallback = OnSuccess;
	OriginalWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	GetCharacterMovement()->MaxWalkSpeed = CastingWalkSpeed;

	UTexture2D* IconTexture = nullptr;
	if (PlayerInventory && PlayerInventory->ItemDataTable)
	{
		if (FItemData* ItemInfo = PlayerInventory->ItemDataTable->FindRow<FItemData>(ItemID, TEXT("GetIcon"))) IconTexture = ItemInfo->ItemIcon;
	}

	if (CastBarWidgetClass)
	{
		if (!CastBarInstance)
		{
			if (APlayerController* PC = Cast<APlayerController>(GetController())) CastBarInstance = CreateWidget<UCastBarWidget>(PC, CastBarWidgetClass);
		}
		if (CastBarInstance && !CastBarInstance->IsInViewport()) CastBarInstance->AddToViewport();
		if (CastBarInstance) CastBarInstance->StartCast(Duration, IconTexture);
	}

	GetWorldTimerManager().SetTimer(CastTimerHandle, [this]() {
		bIsCasting = false;
		GetCharacterMovement()->MaxWalkSpeed = OriginalWalkSpeed;
		if (CastBarInstance && CastBarInstance->IsInViewport()) CastBarInstance->RemoveFromParent();
		if (OnCastSuccessCallback) OnCastSuccessCallback();
		}, Duration, false);
}

void AGun_phiriaCharacter::CancelCasting()
{
	if (!bIsCasting) return;

	bIsCasting = false;
	GetWorldTimerManager().ClearTimer(CastTimerHandle);
	GetCharacterMovement()->MaxWalkSpeed = OriginalWalkSpeed;
	if (CastBarInstance && CastBarInstance->IsInViewport()) CastBarInstance->RemoveFromParent();
}

void AGun_phiriaCharacter::ApplySpeedBuff(float BoostAmount, float Duration)
{
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp) return;

	if (bHasSpeedBuff)
	{
		GetWorldTimerManager().ClearTimer(SpeedBuffTimerHandle);
		if (bIsCasting) OriginalWalkSpeed -= CurrentSpeedBoost;
		else MoveComp->MaxWalkSpeed -= CurrentSpeedBoost;
	}

	bHasSpeedBuff = true;
	CurrentSpeedBoost = BoostAmount;

	if (bIsCasting) OriginalWalkSpeed += CurrentSpeedBoost;
	else MoveComp->MaxWalkSpeed += CurrentSpeedBoost;

	TWeakObjectPtr<AGun_phiriaCharacter> WeakThis(this);
	GetWorldTimerManager().SetTimer(SpeedBuffTimerHandle, [WeakThis]()
		{
			if (WeakThis.IsValid())
			{
				WeakThis->bHasSpeedBuff = false;
				if (WeakThis->bIsCasting) WeakThis->OriginalWalkSpeed -= WeakThis->CurrentSpeedBoost;
				else WeakThis->GetCharacterMovement()->MaxWalkSpeed -= WeakThis->CurrentSpeedBoost;
				WeakThis->CurrentSpeedBoost = 0.0f;
			}
		}, Duration, false);
}

void AGun_phiriaCharacter::ApplyHealOverTime(float TotalHeal, float Duration)
{
	GetWorldTimerManager().ClearTimer(HOTTimerHandle);

	MaxHOTTicks = FMath::FloorToInt(Duration);
	CurrentHOTTicks = 0;
	HOTAmountPerTick = TotalHeal / Duration;

	TWeakObjectPtr<AGun_phiriaCharacter> WeakThis(this);
	GetWorldTimerManager().SetTimer(HOTTimerHandle, [WeakThis]()
		{
			if (WeakThis.IsValid())
			{
				WeakThis->CurrentHealth = FMath::Clamp(WeakThis->CurrentHealth + WeakThis->HOTAmountPerTick, 0.0f, WeakThis->MaxHealth);
				WeakThis->CurrentHOTTicks++;
				if (WeakThis->CurrentHOTTicks >= WeakThis->MaxHOTTicks) WeakThis->GetWorldTimerManager().ClearTimer(WeakThis->HOTTimerHandle);
			}
		}, 1.0f, true);
}

// ==========================================
// Currency
// ==========================================
void AGun_phiriaCharacter::AddGold(int32 Amount) { if (Amount > 0) CurrentGold += Amount; }
bool AGun_phiriaCharacter::SpendGold(int32 Amount) { if (Amount > 0 && CurrentGold >= Amount) { CurrentGold -= Amount; return true; } return false; }
void AGun_phiriaCharacter::ResetGold() { CurrentGold = 0; }
void AGun_phiriaCharacter::AddSapphire(int32 Amount) { if (Amount > 0) CurrentSapphire += Amount; }
bool AGun_phiriaCharacter::SpendSapphire(int32 Amount) { if (Amount > 0 && CurrentSapphire >= Amount) { CurrentSapphire -= Amount; return true; } return false; }
void AGun_phiriaCharacter::CheatCurrency(int32 GoldAmount, int32 SapphireAmount) { AddGold(GoldAmount); AddSapphire(SapphireAmount); }

// ==========================================
// Equipment & Level Transition
// ==========================================
void AGun_phiriaCharacter::UpdateEquipmentVisuals(EEquipType EquipType, UStaticMesh* NewMesh)
{
	switch (EquipType)
	{
	case EEquipType::Helmet: if (HelmetMesh) HelmetMesh->SetStaticMesh(NewMesh); break;
	case EEquipType::Vest: if (VestMesh) VestMesh->SetStaticMesh(NewMesh); break;
	case EEquipType::Backpack: if (BackpackMesh) BackpackMesh->SetStaticMesh(NewMesh); break;
	default: break;
	}

	// 스튜디오 액터에게도 동기화 명령!
	if (SpawnedStudio)
	{
		SpawnedStudio->UpdateStudioEquipment(
			HelmetMesh ? HelmetMesh->GetStaticMesh() : nullptr,
			VestMesh ? VestMesh->GetStaticMesh() : nullptr,
			BackpackMesh ? BackpackMesh->GetStaticMesh() : nullptr,
			CurrentWeapon && CurrentWeapon->GetWeaponMesh() ? CurrentWeapon->GetWeaponMesh()->GetStaticMesh() : nullptr
		);
	}
}

void AGun_phiriaCharacter::RestoreEquipmentVisuals()
{
	if (!PlayerInventory || !PlayerInventory->ItemDataTable) return;

	auto ApplyEquipMesh = [&](FName ItemID, EEquipType EquipType, const TCHAR* Context)
		{
			if (!ItemID.IsNone())
			{
				if (FItemData* ItemData = PlayerInventory->ItemDataTable->FindRow<FItemData>(ItemID, Context)) UpdateEquipmentVisuals(EquipType, ItemData->EquipmentMesh);
			}
		};

	ApplyEquipMesh(PlayerInventory->EquippedHelmetID, EEquipType::Helmet, TEXT("RestoreHelmet"));
	ApplyEquipMesh(PlayerInventory->EquippedVestID, EEquipType::Vest, TEXT("RestoreVest"));
	ApplyEquipMesh(PlayerInventory->EquippedBackpackID, EEquipType::Backpack, TEXT("RestoreBackpack"));
}

void AGun_phiriaCharacter::ForceBlackScreen()
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
		if (PC->PlayerCameraManager) PC->PlayerCameraManager->StartCameraFade(1.0f, 1.0f, 0.0f, FLinearColor::Black, false, true);
}

void AGun_phiriaCharacter::StartFadeIn(float FadeInDuration)
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
		if (PC->PlayerCameraManager) PC->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, FadeInDuration, FLinearColor::Black, false, false);
}