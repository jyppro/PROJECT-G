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
#include "Weapon/ThrowableWeapon.h"

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
	HelmetMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	VestMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VestMesh"));
	VestMesh->SetupAttachment(GetMesh(), FName("VestSocket"));
	VestMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	BackpackMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BackpackMesh"));
	BackpackMesh->SetupAttachment(GetMesh(), FName("BackpackSocket"));
	BackpackMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	MaxHOTTicks = 0;
	CurrentHOTTicks = 0;
	CurrentSpeedBoost = 0.0f;
	bHasSpeedBuff = false;
}

void AGun_phiriaCharacter::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;

	if (StudioClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		FVector StudioLocation = FVector(0.f, 0.f, -10000.f);
		SpawnedStudio = GetWorld()->SpawnActor<AInventoryStudio>(StudioClass, StudioLocation, FRotator::ZeroRotator, SpawnParams);
	}

	// 1. Data Load
	if (UGun_phiriaGameInstance* GameInst = Cast<UGun_phiriaGameInstance>(GetGameInstance()))
	{
		GameInst->LoadPlayerData(this);
		RestoreEquipmentVisuals();
	}

	// 2. Timers & Logic Init
	GetWorldTimerManager().SetTimer(InteractionTimerHandle, this, &AGun_phiriaCharacter::CheckForInteractables, 0.1f, true);
	if (ADSCamera) ADSCamera->AddTickPrerequisiteComponent(GetMesh());

	// 3. Components Init
	InitializeWeapon();
	InitializeInventoryUI();
	RefreshStudioEquipment();
}

// Tick & Tick Helpers
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
	if (!CurrentWeapon) return;

	const bool bIsProneStatus = bIsProne;
	FVector Start = GetActorLocation();

	Start.Z += bIsProneStatus ? 20.0f : 60.0f;

	FVector FwdDir = GetActorForwardVector();

	float TraceDistance = bIsProneStatus ? 55.0f : 65.0f;
	FVector End = Start + (FwdDir * TraceDistance);

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(CurrentWeapon);

	FCollisionShape Sphere = FCollisionShape::MakeSphere(bIsProneStatus ? 7.0f : 12.0f);

	float TargetAlpha = 0.0f;

	if (GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Visibility, Sphere, Params))
	{
		if (bIsProneStatus && FMath::Abs(Hit.ImpactNormal.Z) > 0.7f)
		{
			TargetAlpha = 0.0f;
		}
		else
		{
			TargetAlpha = 1.0f;
		}
	}

	float InterpSpeed = (TargetAlpha > WeaponObstructionAlpha) ? 20.0f : 10.0f;

	WeaponObstructionAlpha = FMath::FInterpTo(WeaponObstructionAlpha, TargetAlpha, GetWorld()->GetDeltaSeconds(), InterpSpeed);
}

void AGun_phiriaCharacter::InitializeWeapon()
{
	WeaponSlots.Init(nullptr, 4);
	if (!PlayerInventory || !PlayerInventory->ItemDataTable) return;

	// [추가] 시작할 때 미리 GameInstance를 가져와 둡니다.
	UGun_phiriaGameInstance* GameInst = Cast<UGun_phiriaGameInstance>(GetGameInstance());

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;

	// 1. [0번 슬롯: 권총]
	FName PistolID = PlayerInventory->EquippedPistolID.IsNone() ? FName("DefaultPistol") : PlayerInventory->EquippedPistolID;

	if (FItemData* PistolData = PlayerInventory->ItemDataTable->FindRow<FItemData>(PistolID, TEXT("SpawnPistol"))) {
		if (PistolData->WeaponClass) {
			WeaponSlots[0] = GetWorld()->SpawnActor<AWeaponBase>(PistolData->WeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
			if (WeaponSlots[0]) WeaponSlots[0]->HolsterRotationOffset = PistolData->HolsterRotationOffset;
		}
	}

	if (!WeaponSlots[0] && DefaultWeaponClass) {
		WeaponSlots[0] = GetWorld()->SpawnActor<AWeaponBase>(DefaultWeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	}

	AttachToHolster(0);

	// 2. [1번 슬롯: 주무기 1]
	if (!PlayerInventory->EquippedWeapon1ID.IsNone()) {
		if (FItemData* W1Data = PlayerInventory->ItemDataTable->FindRow<FItemData>(PlayerInventory->EquippedWeapon1ID, TEXT("SpawnW1"))) {
			if (W1Data->WeaponClass) {
				WeaponSlots[1] = GetWorld()->SpawnActor<AWeaponBase>(W1Data->WeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

				if (WeaponSlots[1]) {
					WeaponSlots[1]->HolsterRotationOffset = W1Data->HolsterRotationOffset;

					// [추가] 주무기1 탄창 복구!
					if (GameInst && GameInst->bHasSavedData) {
						WeaponSlots[1]->CurrentAmmo = GameInst->SavedWeapon1Ammo;
					}
				}
			}
		}
	}

	// 3. [2번 슬롯: 주무기 2]
	if (!PlayerInventory->EquippedWeapon2ID.IsNone()) {
		if (FItemData* W2Data = PlayerInventory->ItemDataTable->FindRow<FItemData>(PlayerInventory->EquippedWeapon2ID, TEXT("SpawnW2"))) {
			if (W2Data->WeaponClass) {
				WeaponSlots[2] = GetWorld()->SpawnActor<AWeaponBase>(W2Data->WeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

				if (WeaponSlots[2]) {
					WeaponSlots[2]->HolsterRotationOffset = W2Data->HolsterRotationOffset;

					// [추가] 주무기2 탄창 복구!
					if (GameInst && GameInst->bHasSavedData) {
						WeaponSlots[2]->CurrentAmmo = GameInst->SavedWeapon2Ammo;
					}
				}
			}
		}
	}

	AttachToHolster(2);

	if (!PlayerInventory->EquippedThrowableID.IsNone())
	{
		if (FItemData* TData = PlayerInventory->ItemDataTable->FindRow<FItemData>(PlayerInventory->EquippedThrowableID, TEXT("SpawnThrowable")))
		{
			if (TData->WeaponClass)
			{
				WeaponSlots[3] = GetWorld()->SpawnActor<AWeaponBase>(TData->WeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

				if (WeaponSlots[3])
				{
					WeaponSlots[3]->HolsterRotationOffset = TData->HolsterRotationOffset;

					// 수류탄은 탄약이 곧 인벤토리 개수입니다. 인벤토리를 뒤져서 총 개수를 구합니다.
					int32 ThrowableCount = 0;
					for (const FInventorySlot& Slot : PlayerInventory->InventorySlots)
					{
						if (Slot.ItemID == PlayerInventory->EquippedThrowableID)
						{
							ThrowableCount += Slot.Quantity;
						}
					}
					WeaponSlots[3]->CurrentAmmo = ThrowableCount;
				}
			}
		}
	}

	// 투척 무기도 홀스터(3번 인덱스)에 부착합니다.
	AttachToHolster(3);

	// 4. 활성 슬롯 무기 꺼내기
	if (ActiveWeaponSlot < 0 || ActiveWeaponSlot >= 4 || WeaponSlots[ActiveWeaponSlot] == nullptr) ActiveWeaponSlot = 0;
	CurrentWeapon = nullptr;
	EquipWeaponSlot(ActiveWeaponSlot);
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

// Input Bindings & Movement
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

		if (EquipWeapon1Action) EIC->BindAction(EquipWeapon1Action, ETriggerEvent::Started, this, &AGun_phiriaCharacter::EquipWeapon1);
		if (EquipWeapon2Action) EIC->BindAction(EquipWeapon2Action, ETriggerEvent::Started, this, &AGun_phiriaCharacter::EquipWeapon2);
		if (EquipWeapon3Action) EIC->BindAction(EquipWeapon3Action, ETriggerEvent::Started, this, &AGun_phiriaCharacter::EquipWeapon3);
		if (EquipWeapon4Action) EIC->BindAction(EquipWeapon4Action, ETriggerEvent::Started, this, &AGun_phiriaCharacter::EquipWeapon4);

		if (ReloadAction) EIC->BindAction(ReloadAction, ETriggerEvent::Started, this, &AGun_phiriaCharacter::Reload);
	}
}

void AGun_phiriaCharacter::Move(const FInputActionValue& Value)
{
	if (!Controller) return;

	const FVector2D Vec = Value.Get<FVector2D>();
	const FRotator Rot = FRotator(0, Controller->GetControlRotation().Yaw, 0);
	const FVector Fwd = FRotationMatrix(Rot).GetUnitAxis(EAxis::X);
	const FVector Rit = FRotationMatrix(Rot).GetUnitAxis(EAxis::Y);

	AddMovementInput(Fwd, Vec.Y);
	AddMovementInput(Rit, Vec.X);
}

void AGun_phiriaCharacter::EquipWeapon1() { EquipWeaponSlot(0); } // 권총
void AGun_phiriaCharacter::EquipWeapon2() { EquipWeaponSlot(1); } // 주무기 1
void AGun_phiriaCharacter::EquipWeapon3() { EquipWeaponSlot(2); } // 주무기 2
void AGun_phiriaCharacter::EquipWeapon4() { EquipWeaponSlot(3); } // 투척물

void AGun_phiriaCharacter::EquipWeaponSlot(int32 SlotIndex)
{
	if (bIsCasting || bIsDead || bIsInventoryOpen || bIsReloading || SlotIndex < 0 || SlotIndex >= WeaponSlots.Num()) return;

	AWeaponBase* TargetWeapon = WeaponSlots[SlotIndex];
	if (!TargetWeapon || TargetWeapon == CurrentWeapon) return;

	// 1. 기존 무기를 확실하게 등에 있는 '보관 소켓'으로 보내기
	if (CurrentWeapon)
	{
		StopFiring();
		if (bIsAiming) StopAiming();

		// [보강] 기존 무기의 충돌을 끄고 물리 영향을 받지 않게 함 (겹침 방지)
		CurrentWeapon->SetActorEnableCollision(false);

		// 기존 무기를 등에 붙입니다.
		AttachToHolster(ActiveWeaponSlot);
	}

	// 2. 새 무기(수류탄 포함) 정보 갱신
	ActiveWeaponSlot = SlotIndex;
	CurrentWeapon = TargetWeapon;

	// 새 무기가 나타나도록 설정
	CurrentWeapon->SetActorHiddenInGame(false);

	// [중요] 손에 쥐고 있을 때는 충돌을 꺼야 캐릭터 이동이나 다른 무기와 겹쳐서 튕기는 걸 막습니다.
	CurrentWeapon->SetActorEnableCollision(false);

	// 손으로 가져와서 부착 (SnapToTarget으로 확실하게 위치 고정)
	FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, true);
	CurrentWeapon->AttachToComponent(GetMesh(), AttachRules, FName("WeaponSocket"));

	// [크기 버그 해결]
	CurrentWeapon->SetActorRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));

	if (CurrentWeapon->GetWeaponMesh())
	{
		// 역행렬 그립 보정 로직
		FTransform GripSocketRelative = CurrentWeapon->GetWeaponMesh()->GetSocketTransform(FName("RightHandGripSocket"), ERelativeTransformSpace::RTS_Actor);
		GripSocketRelative.SetScale3D(FVector(1.0f, 1.0f, 1.0f));
		FTransform InverseGrip = GripSocketRelative.Inverse();

		CurrentWeapon->SetActorRelativeLocation(InverseGrip.GetLocation());
		CurrentWeapon->SetActorRelativeRotation(InverseGrip.GetRotation());

		// 투척 무기는 조준경(SightSocket)이 없을 수 있으므로 체크 후 부착
		if (ADSCamera && CurrentWeapon->GetWeaponMesh()->DoesSocketExist(FName("SightSocket")))
		{
			ADSCamera->AttachToComponent(CurrentWeapon->GetWeaponMesh(), AttachRules, FName("SightSocket"));
		}
		else if (ADSCamera)
		{
			// 투척 무기일 때는 카메라를 다시 원래 위치(메쉬)로 복구하거나 그대로 둡니다.
			ADSCamera->AttachToComponent(GetMesh(), AttachRules);
		}
	}

	RefreshStudioEquipment();
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
	if (bIsCasting || bIsChangingStance || bIsReloading) return;
	if (bIsCasting) { CancelCasting(); return; }
	if (bIsChangingStance) return;

	if (bIsProne) ToggleProne();
	else if (bIsCrouched) UnCrouch();
	else Jump();
}

void AGun_phiriaCharacter::ToggleCrouch()
{
	if (bIsCasting || bIsReloading) return;
	if (bIsCasting) { CancelCasting(); return; }
	if (!bIsProne) bIsCrouched ? UnCrouch() : Crouch();
}

void AGun_phiriaCharacter::ToggleProne()
{
	if (bIsCasting || GetCharacterMovement()->IsFalling() || bIsReloading) return;
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

// Combat
void AGun_phiriaCharacter::StartAiming()
{
	if (bIsInventoryOpen || bIsCasting || bIsReloading) return;
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
	if (bIsInventoryOpen || bIsCasting || bIsReloading) return;
	if (bIsInventoryOpen || bIsCasting) { CancelCasting(); return; }
	if (!CurrentWeapon || !FollowCamera || !ADSCamera) return;

	if (AThrowableWeapon* ThrowableWep = Cast<AThrowableWeapon>(CurrentWeapon))
	{
		// StartCooking() 대신 부모를 오버라이드한 Fire()를 호출합니다.
		// (투척 무기 쿠킹에는 타겟 위치가 필요 없으므로 ZeroVector를 넘겨줍니다)
		ThrowableWep->Fire(FVector::ZeroVector);

		bIsAimingThrowable = true;
		return;
	}

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
	if (AThrowableWeapon* ThrowableWep = Cast<AThrowableWeapon>(CurrentWeapon))
	{
		bIsAimingThrowable = false; // 궤적 그리기 끄기

		// 던질 방향을 카메라가 바라보는 방향으로 설정 (배그와 동일)
		FVector CameraLocation = FollowCamera->GetComponentLocation();
		FVector ThrowDirection = FollowCamera->GetForwardVector();

		// 실제 투척 실행!
		ThrowableWep->ReleaseThrow(CameraLocation, ThrowDirection);
		return;
	}

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

// Health & Damage
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

// Interaction & Inventory
void AGun_phiriaCharacter::CheckForInteractables()
{
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params; Params.AddIgnoredActor(this);
	TObjectPtr<AActor> BestTarget = nullptr;

	if (GetWorld()->OverlapMultiByChannel(Overlaps, GetActorLocation(), FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(100.f), Params))
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
	if (bIsReloading) return;

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

	if (GetWorld()->OverlapMultiByChannel(Overlaps, GetActorLocation(), FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(100.f), Params))
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
		// 1. 캐릭터 앞쪽으로 스폰할 기준 위치 잡기 (살짝 위에서 시작)
		FVector StartLoc = GetActorLocation() + (GetActorForwardVector() * 80.0f);

		// 2. 레이저가 도착할 목표 지점 (기준 위치에서 아래로 500만큼 쏩니다)
		FVector EndLoc = StartLoc - FVector(0.0f, 0.0f, 500.0f);

		FHitResult HitResult;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this); // 플레이어 몸에는 레이저가 맞지 않게 무시!

		// 최종 스폰될 위치 변수
		FVector FinalSpawnLoc = StartLoc;

		// 3. 바닥을 향해 레이저(LineTrace) 발사!
		if (GetWorld()->LineTraceSingleByChannel(HitResult, StartLoc, EndLoc, ECC_Visibility, Params))
		{
			// 레이저가 바닥에 맞았다면? -> 닿은 지점(ImpactPoint)을 최종 위치로 결정
			// (팁: 아이템이 바닥에 너무 파묻히면 ImpactPoint.Z + 5.0f 처럼 살짝 올려주세요)
			FinalSpawnLoc = HitResult.ImpactPoint;
		}
		else
		{
			// 혹시라도 낭떠러지 등이라 바닥을 못 찾았다면 기존처럼 대충 아래로 내림
			FinalSpawnLoc.Z -= 80.0f;
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		// 4. 찾아낸 정확한 바닥 위치(FinalSpawnLoc)에 아이템 생성!
		APickupItemBase* DroppedItem = GetWorld()->SpawnActor<APickupItemBase>(ItemData->ItemClass, FinalSpawnLoc, FRotator::ZeroRotator, SpawnParams);

		if (DroppedItem)
		{
			DroppedItem->ItemID = ItemID;
			DroppedItem->Quantity = 1;
		}
	}

	RefreshStudioEquipment();
}

// Casting & Buffs
void AGun_phiriaCharacter::StartCasting(float Duration, FName ItemID, TFunction<void()> OnSuccess)
{
	if (bIsReloading) return;
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

				WeakThis->GetWorldTimerManager().ClearTimer(WeakThis->SpeedBuffTimerHandle);
			}
		}, Duration, false);
}

void AGun_phiriaCharacter::ApplyHealOverTime(float TotalHeal, float Duration)
{
	GetWorldTimerManager().ClearTimer(HOTTimerHandle);

	MaxHOTTicks = FMath::FloorToInt(Duration);
	CurrentHOTTicks = MaxHOTTicks; // [수정] 0이 아니라 Max에서 시작! (100% 상태)
	HOTAmountPerTick = TotalHeal / Duration;

	TWeakObjectPtr<AGun_phiriaCharacter> WeakThis(this);
	GetWorldTimerManager().SetTimer(HOTTimerHandle, [WeakThis]()
		{
			if (WeakThis.IsValid())
			{
				WeakThis->CurrentHealth = FMath::Clamp(WeakThis->CurrentHealth + WeakThis->HOTAmountPerTick, 0.0f, WeakThis->MaxHealth);

				// [수정] 틱을 감소시킵니다.
				WeakThis->CurrentHOTTicks--;

				// [수정] 틱이 0 이하가 되면(버프 종료)
				if (WeakThis->CurrentHOTTicks <= 0)
				{
					WeakThis->GetWorldTimerManager().ClearTimer(WeakThis->HOTTimerHandle);
					// CurrentHOTTicks가 0이 되었으므로 HUD에서 즉시 UI를 숨김!
				}
			}
		}, 1.0f, true);
}

// Currency
void AGun_phiriaCharacter::AddGold(int32 Amount) { if (Amount > 0) CurrentGold += Amount; }
bool AGun_phiriaCharacter::SpendGold(int32 Amount) { if (Amount > 0 && CurrentGold >= Amount) { CurrentGold -= Amount; return true; } return false; }
void AGun_phiriaCharacter::ResetGold() { CurrentGold = 0; }
void AGun_phiriaCharacter::AddSapphire(int32 Amount) { if (Amount > 0) CurrentSapphire += Amount; }
bool AGun_phiriaCharacter::SpendSapphire(int32 Amount) { if (Amount > 0 && CurrentSapphire >= Amount) { CurrentSapphire -= Amount; return true; } return false; }
void AGun_phiriaCharacter::CheatCurrency(int32 GoldAmount, int32 SapphireAmount) { AddGold(GoldAmount); AddSapphire(SapphireAmount); }

// Equipment & Level Transition
void AGun_phiriaCharacter::UpdateEquipmentVisuals(EEquipType EquipType, UStaticMesh* NewMesh)
{
	switch (EquipType)
	{
	case EEquipType::Helmet: if (HelmetMesh) HelmetMesh->SetStaticMesh(NewMesh); break;
	case EEquipType::Vest: if (VestMesh) VestMesh->SetStaticMesh(NewMesh); break;
	case EEquipType::Backpack: if (BackpackMesh) BackpackMesh->SetStaticMesh(NewMesh); break;
	default: break;
	}

	RefreshStudioEquipment();
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

void AGun_phiriaCharacter::Reload()
{
	// 1. 기존 예외 처리 유지
	if (bIsReloading || bIsCasting || bIsDead || bIsInventoryOpen) return;
	if (!CurrentWeapon || !PlayerInventory) return;
	if (CurrentWeapon->CurrentAmmo >= CurrentWeapon->MagazineCapacity) return;

	FName NeededAmmoID = CurrentWeapon->AmmoItemID;
	int32 TotalReserveAmmo = PlayerInventory->GetTotalItemCount(NeededAmmoID);
	if (TotalReserveAmmo <= 0) return;

	if (bIsAiming) StopAiming();

	OriginalWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	GetCharacterMovement()->MaxWalkSpeed = CastingWalkSpeed;

	bIsReloading = true;

	if (CurrentWeapon->ReloadMontage)
	{
		if (TObjectPtr<UAnimInstance> AnimInst = GetMesh()->GetAnimInstance())
		{
			// 1. 몽타주 재생을 요청하고, 그 전체 길이를 가져옵니다.
			float ReloadDuration = AnimInst->Montage_Play(CurrentWeapon->ReloadMontage);

			// 2. [추가] 장전 시간(ReloadDuration)만큼 캐스트 바를 화면에 띄웁니다!
			if (CastBarWidgetClass)
			{
				if (!CastBarInstance)
				{
					if (APlayerController* PC = Cast<APlayerController>(GetController()))
					{
						CastBarInstance = CreateWidget<UCastBarWidget>(PC, CastBarWidgetClass);
					}
				}

				if (CastBarInstance && !CastBarInstance->IsInViewport())
				{
					CastBarInstance->AddToViewport();
				}

				if (CastBarInstance)
				{
					// 아이콘 자리에 nullptr을 넘겨주어 아이콘 없이 타이머만 돌아가게 합니다.
					CastBarInstance->StartCast(ReloadDuration, nullptr);
				}
			}
		}
	}
	else
	{
		FinishReload();
	}
}

void AGun_phiriaCharacter::FinishReload()
{
	// [추가] 장전 완료: 이동 속도를 원래대로 복구합니다.
	if (bIsReloading) // 장전 중일 때만 복구 (버그 방지)
	{
		GetCharacterMovement()->MaxWalkSpeed = OriginalWalkSpeed;

		if (CastBarInstance && CastBarInstance->IsInViewport())
		{
			CastBarInstance->RemoveFromParent();
		}
	}

	if (!CurrentWeapon || !PlayerInventory)
	{
		bIsReloading = false;
		return;
	}

	FName NeededAmmoID = CurrentWeapon->AmmoItemID;
	int32 TotalReserveAmmo = PlayerInventory->GetTotalItemCount(NeededAmmoID);

	int32 BulletsNeeded = CurrentWeapon->MagazineCapacity - CurrentWeapon->CurrentAmmo;
	int32 BulletsToReload = FMath::Min(BulletsNeeded, TotalReserveAmmo);

	if (BulletsToReload > 0)
	{
		PlayerInventory->RemoveItem(NeededAmmoID, BulletsToReload);
		CurrentWeapon->CurrentAmmo += BulletsToReload;
	}

	// 장전 상태 해제
	bIsReloading = false;
}

void AGun_phiriaCharacter::AttachToHolster(int32 SlotIndex)
{
	// 예외 처리
	if (!WeaponSlots.IsValidIndex(SlotIndex) || !WeaponSlots[SlotIndex]) return;
	AWeaponBase* WeaponToHolster = WeaponSlots[SlotIndex];

	// 슬롯 번호에 맞는 정확한 보관 소켓 이름 지정
	FName HolsterSocketName;
	if (SlotIndex == 0) HolsterSocketName = FName("PistolHolsterSocket");
	else if (SlotIndex == 1) HolsterSocketName = FName("BackpackWeapon1Socket");
	else if (SlotIndex == 2) HolsterSocketName = FName("BackpackWeapon2Socket");
	else return; // 투척물 등은 제외

	// 부착 룰 (스케일 유지)
	FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepRelative, true);

	// 무기 액터를 해당 소켓에 부착
	WeaponToHolster->AttachToComponent(GetMesh(), AttachRules, HolsterSocketName);

	// [핵심] 데이터 테이블의 회전값 적용 및 스케일 1.0 강제 고정
	WeaponToHolster->SetActorRelativeRotation(WeaponToHolster->HolsterRotationOffset);
	WeaponToHolster->SetActorRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));

	RefreshStudioEquipment();
}

void AGun_phiriaCharacter::RefreshStudioEquipment()
{
	// 스튜디오가 띄워져 있지 않다면 무시
	if (!SpawnedStudio) return;

	EStudioAnimType AnimState = EStudioAnimType::Idle;

	if (CurrentWeapon && WeaponSlots.IsValidIndex(0) && CurrentWeapon != WeaponSlots[0])
	{
		AnimState = EStudioAnimType::Rifle;
	}

	SpawnedStudio->UpdateStudioEquipment(
		HelmetMesh ? HelmetMesh->GetStaticMesh() : nullptr,
		VestMesh ? VestMesh->GetStaticMesh() : nullptr,
		BackpackMesh ? BackpackMesh->GetStaticMesh() : nullptr,
		CurrentWeapon,
		AnimState
	);
}