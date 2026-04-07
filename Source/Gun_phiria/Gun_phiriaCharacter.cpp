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

// Engine Headers
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

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

AGun_phiriaCharacter::AGun_phiriaCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Collision & Physics
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	// Rotation & Movement
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

	// Initial States
	bIsProne = false;
	DefaultCapsuleHalfHeight = 96.0f;
	DefaultMeshRelativeLocationZ = -97.0f;

	PlayerInventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("PlayerInventory"));

	// --- 생성자 마지막 부분에 추가 ---
	// 1. 인벤토리 카메라 생성
	InventoryCamera = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("InventoryCamera"));
	// 2. 캐릭터 루트(보통 캡슐)에 붙임
	InventoryCamera->SetupAttachment(RootComponent);

	InventoryCamera->SetRelativeLocation(FVector(150.0f, 0.0f, 0.0f));
	InventoryCamera->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	InventoryCamera->FOVAngle = 80.0f;

	// 1. UI용 가짜 몸통 생성 및 부착
	InventoryCloneMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("InventoryCloneMesh"));
	InventoryCloneMesh->SetupAttachment(RootComponent);

	InventoryCloneMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
	InventoryCloneMesh->SetRelativeRotation(FRotator(0.0f, 255.0f, 0.0f));

	InventoryCloneMesh->bVisibleInSceneCaptureOnly = true;
	InventoryCloneMesh->SetCastShadow(false);

	HelmetMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HelmetMesh"));
	HelmetMesh->SetupAttachment(GetMesh(), FName("HelmetSocket"));

	VestMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VestMesh"));
	VestMesh->SetupAttachment(GetMesh(), FName("VestSocket"));

	BackpackMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BackpackMesh"));
	BackpackMesh->SetupAttachment(GetMesh(), FName("BackpackSocket")); // 방금 만든 소켓 이름

	CloneHelmetMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CloneHelmetMesh"));
	CloneHelmetMesh->SetupAttachment(InventoryCloneMesh, FName("HelmetSocket"));
	CloneHelmetMesh->bVisibleInSceneCaptureOnly = true;

	CloneVestMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CloneVestMesh"));
	CloneVestMesh->SetupAttachment(InventoryCloneMesh, FName("VestSocket"));
	CloneVestMesh->bVisibleInSceneCaptureOnly = true;

	CloneBackpackMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CloneBackpackMesh"));
	CloneBackpackMesh->SetupAttachment(InventoryCloneMesh, FName("BackpackSocket"));
	CloneBackpackMesh->bVisibleInSceneCaptureOnly = true;

	CloneWeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CloneWeaponMesh"));
	CloneWeaponMesh->SetupAttachment(InventoryCloneMesh, FName("WeaponSocket")); // 실제 무기가 붙는 소켓과 동일한 이름!
	CloneWeaponMesh->bVisibleInSceneCaptureOnly = true;
	CloneWeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // UI용이므로 충돌 제거
}

void AGun_phiriaCharacter::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;

	// [데이터 불러오기!]
	if (UGun_phiriaGameInstance* GameInst = Cast<UGun_phiriaGameInstance>(GetGameInstance()))
	{
		// 1. 인벤토리 데이터(ID 등) 복구
		GameInst->LoadPlayerData(this);

		// 2. [추가된 부분] 복구된 ID를 바탕으로 3D 외형을 다시 입습니다!
		RestoreEquipmentVisuals();
	}

	// Interaction Timer
	GetWorldTimerManager().SetTimer(InteractionTimerHandle, this, &AGun_phiriaCharacter::CheckForInteractables, 0.1f, true);

	// Weapon Initialization
	if (DefaultWeaponClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		CurrentWeapon = GetWorld()->SpawnActor<AWeaponBase>(DefaultWeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

		if (CurrentWeapon)
		{
			FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
			CurrentWeapon->AttachToComponent(GetMesh(), AttachmentRules, FName("WeaponSocket"));
			ADSCamera->AttachToComponent(CurrentWeapon->GetWeaponMesh(), AttachmentRules, FName("SightSocket"));

			if (CloneWeaponMesh && CurrentWeapon->GetWeaponMesh())
			{
				CloneWeaponMesh->SetStaticMesh(CurrentWeapon->GetWeaponMesh()->GetStaticMesh());
			}
		}
	}

	if (ADSCamera) ADSCamera->AddTickPrerequisiteComponent(GetMesh());

	// --- 인벤토리 UI 생성 및 숨김 처리 ---
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

	if (InventoryCamera)
	{
		// [핵심] 카메라가 "내가 지정한 컴포넌트만 찍겠다(ShowOnlyList)"고 선언하는 필수 설정입니다.
		InventoryCamera->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
		InventoryCamera->ShowOnlyComponent(InventoryCloneMesh);
		if (CloneHelmetMesh) InventoryCamera->ShowOnlyComponent(CloneHelmetMesh);
		if (CloneVestMesh) InventoryCamera->ShowOnlyComponent(CloneVestMesh);
		if (CloneBackpackMesh) InventoryCamera->ShowOnlyComponent(CloneBackpackMesh);
		if (CloneWeaponMesh) InventoryCamera->ShowOnlyComponent(CloneWeaponMesh);
	}
}

void AGun_phiriaCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 1. Camera FOV Interpolation
	TObjectPtr<UCameraComponent> ActiveCam = bIsAiming ? ADSCamera : FollowCamera;
	float TargetFOV = bIsAiming ? AimFOV : DefaultFOV;
	ActiveCam->SetFieldOfView(FMath::FInterpTo(ActiveCam->FieldOfView, TargetFOV, DeltaTime, ZoomInterpSpeed));

	// 2. Dynamic Spread Calculation
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

	// 3. Procedural ADS Offset
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

	// 4. Locomotion Variables
	const FVector Velocity = GetVelocity();
	MovementDirectionAngle = (Velocity.Size2D() > 1.0f) ? GetActorTransform().InverseTransformVectorNoScale(Velocity).Rotation().Yaw : 0.0f;

	const float CurrentYaw = GetActorRotation().Yaw;
	float YawDelta = FRotator::NormalizeAxis(CurrentYaw - PreviousActorYaw);
	YawRotationSpeed = YawDelta / DeltaTime;
	PreviousActorYaw = CurrentYaw;

	// 5. Headshot Detection
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

	// 6. Lean & Camera Offset
	CurrentLean = FMath::FInterpTo(CurrentLean, TargetLean, DeltaTime, 10.0f);
	LeanAxisCS = GetMesh()->GetComponentTransform().InverseTransformVectorNoScale(GetBaseAimRotation().Vector());

	if (CameraBoom)
	{
		float H = bIsProne ? 60.f : (bIsCrouched ? 90.f : 120.f);
		CameraBoom->SocketOffset = FMath::VInterpTo(CameraBoom->SocketOffset, FVector(0.f, CurrentLean * 100.f, H), DeltaTime, 10.0f);
	}

	// 7. Weapon Collision Prevention
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

void AGun_phiriaCharacter::StartAiming()
{
	if (bIsInventoryOpen) return;

	// 아이템 시전 중 조준을 시도하면 시전 취소
	if (bIsCasting)
	{
		CancelCasting();
		return;
	}

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
	if (bIsInventoryOpen) return;

	if (bIsCasting)
	{
		CancelCasting();
		return;
	}

	if (!CurrentWeapon || !FollowCamera || !ADSCamera) return;

	bIsFiring = true;
	GetWorldTimerManager().SetTimer(FireTimerHandle, this, &AGun_phiriaCharacter::StopFiringPose, 1.0f, false);
	LastFireTime = GetWorld()->GetTimeSeconds();

	// Animation
	if (TObjectPtr<UAnimInstance> AnimInst = GetMesh()->GetAnimInstance())
	{
		const auto& Montages = bIsProne ? CurrentWeapon->ProneFireMontages : CurrentWeapon->StandCrouchFireMontages;
		if (Montages.Num() > 0) AnimInst->Montage_Play(Montages[FMath::RandRange(0, Montages.Num() - 1)]);
	}

	// Recoil & Spread
	float Mult = (bIsAiming ? 0.6f : 1.2f) * (GetVelocity().Size2D() > 10.f ? 1.5f : 1.f) * (GetCharacterMovement()->IsFalling() ? 2.5f : 1.f);
	Mult *= bIsProne ? 0.4f : (bIsCrouched ? 0.75f : 1.f);

	AddControllerPitchInput(FMath::RandRange(-0.5f, -1.0f) * Mult);
	AddControllerYawInput(FMath::RandRange(-0.5f, 0.5f) * Mult);
	CurrentSpread = FMath::Clamp(CurrentSpread + (SpreadPerShot * Mult), 0.0f, MaxSpread);

	// Line Trace Logic
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

	// Apply Spread
	FVector Right, Up; RealDir.FindBestAxisVectors(Right, Up);
	const float Radius = FMath::Tan(FMath::DegreesToRadians(CurrentSpread * CurrentWeapon->WeaponSpreadMultiplier)) * FVector::Distance(Muzzle, FinalTarget);
	const float RandAngle = FMath::RandRange(0.f, PI * 2.f);
	const FVector Offset = (Right * FMath::Cos(RandAngle) + Up * FMath::Sin(RandAngle)) * FMath::RandRange(0.f, Radius);

	CurrentWeapon->Fire(FinalTarget + Offset);
}

void AGun_phiriaCharacter::StopFiringPose() { bIsFiring = false; }

float AGun_phiriaCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead) return 0.0f;

	float ActualDamage = DamageAmount;
	bool bIsHeadshot = false;
	FName HitBoneName = NAME_None;

	// 1. 맞은 부위(뼈 이름) 확인 및 헤드샷 판별
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		HitBoneName = static_cast<const FPointDamageEvent*>(&DamageEvent)->HitInfo.BoneName;

		// 화면에 맞은 부위 출력 (흰색)
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::White, FString::Printf(TEXT("Hit Part: %s / Damage: %.1f"), *HitBoneName.ToString(), ActualDamage));

		if (HitBoneName == FName("head"))
		{
			ActualDamage *= 2.5f;
			bIsHeadshot = true;
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, FString::Printf(TEXT("!!! HeadShot !!! Powerful Damage: %.1f"), ActualDamage));
		}
	}

	// ==========================================
	// 2. 장비(조끼/헬멧) 방어력 및 내구도 적용 로직 (배틀그라운드 방식)
	// ==========================================

	// 방어구에 의해 깎이기 전의 '원본 데미지'를 기억해둡니다.
	float OriginalDamage = ActualDamage;

	if (PlayerInventory)
	{
		if (bIsHeadshot && !PlayerInventory->EquippedHelmetID.IsNone())
		{
			if (FItemData* HelmetData = PlayerInventory->ItemDataTable->FindRow<FItemData>(PlayerInventory->EquippedHelmetID, TEXT("HelmetDefense")))
			{
				// 방어율(DefensePower)만큼 데미지 감소
				float BlockedDamage = OriginalDamage * HelmetData->DefensePower;
				ActualDamage -= BlockedDamage;

				// 내구도는 적의 원본 데미지(OriginalDamage)만큼 통째로 깎임
				PlayerInventory->CurrentHelmetDurability -= OriginalDamage;

				if (GEngine)
				{
					FString DebugMsg = FString::Printf(TEXT("[Helmet Defense] %.1f Damage Blocked! / Remaining Durability: %.1f"), BlockedDamage, PlayerInventory->CurrentHelmetDurability);
					GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Cyan, DebugMsg);
				}

				// 헬멧 파괴 처리
				if (PlayerInventory->CurrentHelmetDurability <= 0.0f)
				{
					PlayerInventory->MaxWeight -= HelmetData->StatBonus;
					PlayerInventory->EquippedHelmetID = NAME_None;
					PlayerInventory->CurrentHelmetDurability = 0.0f;
					UpdateEquipmentVisuals(EEquipType::Helmet, nullptr);

					if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, TEXT("!!! Helmet Destroyed !!!"));

					// 인벤토리가 열려있을 때 파괴되면 UI 즉시 갱신
					if (bIsInventoryOpen && InventoryWidgetInstance)
					{
						if (UInventoryMainWidget* MainWidget = Cast<UInventoryMainWidget>(InventoryWidgetInstance))
						{
							MainWidget->UpdateEquipmentUI();
						}
					}
				}
			}
		}
		// 기존 조끼 로직 (헤드샷이 아닐 때)
		else if (!bIsHeadshot && !PlayerInventory->EquippedVestID.IsNone())
		{
			if (FItemData* VestData = PlayerInventory->ItemDataTable->FindRow<FItemData>(PlayerInventory->EquippedVestID, TEXT("VestDefense")))
			{
				// [배그 매커니즘 1] 내구도가 1이라도 남아있다면, 정해진 방어율(예: 55%)만큼 무조건 데미지를 깎아줍니다.
				float BlockedDamage = OriginalDamage * VestData->DefensePower;
				ActualDamage -= BlockedDamage;

				// [배그 매커니즘 2] 조끼의 내구도는 '막아낸 수치'가 아니라, '적의 원본 데미지(OriginalDamage)'만큼 통째로 깎입니다.
				PlayerInventory->CurrentVestDurability -= OriginalDamage;

				// 디버그 출력 (청록색)
				if (GEngine)
				{
					FString DebugMsg = FString::Printf(TEXT("[Vest Defense] % .1f Damage Blocked!/ Remaining Durability : % .1f"), BlockedDamage, PlayerInventory->CurrentVestDurability);
					GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Cyan, DebugMsg);
				}

				if (PlayerInventory->CurrentVestDurability <= 0.0f)
				{
					PlayerInventory->MaxWeight -= VestData->StatBonus;
					PlayerInventory->EquippedVestID = NAME_None;
					PlayerInventory->CurrentVestDurability = 0.0f;
					UpdateEquipmentVisuals(EEquipType::Vest, nullptr);

					if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, TEXT("!!! Vest Destroyed !!!"));

					// ==========================================
					// [여기에 추가] 인벤토리가 열려있을 때 파괴되면 UI 즉시 갱신
					// ==========================================
					if (bIsInventoryOpen && InventoryWidgetInstance)
					{
						if (UInventoryMainWidget* MainWidget = Cast<UInventoryMainWidget>(InventoryWidgetInstance))
						{
							MainWidget->UpdateEquipmentUI(); // 조끼가 파괴되었으니 아이콘을 즉시 지움!
						}
					}
				}
			}
		}
	}
	// ==========================================

	// 3. 최종 데미지 적용
	ActualDamage = Super::TakeDamage(ActualDamage, DamageEvent, EventInstigator, DamageCauser);
	if (HitMontages.Num() > 0) PlayAnimMontage(HitMontages[FMath::RandRange(0, HitMontages.Num() - 1)]);

	CurrentHealth = FMath::Clamp(CurrentHealth - ActualDamage, 0.0f, MaxHealth);

	// 화면에 최종 결과 출력 (빨간색)
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, FString::Printf(TEXT("-> Player Final Attack Damage: %.1f / Remain Health: %.1f"), ActualDamage, CurrentHealth));

	if (CurrentHealth <= 0.0f) Die();

	return ActualDamage;
}

void AGun_phiriaCharacter::Die()
{
	if (bIsDead) return;
	bIsDead = true;
	DisableInput(Cast<APlayerController>(GetController()));
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	GetMesh()->SetSimulatePhysics(true);
}

void AGun_phiriaCharacter::ToggleCrouch()
{
	// 시전 중 앉기/일어서기 시도 시 취소
	if (bIsCasting)
	{
		CancelCasting();
		return;
	}
	if (!bIsProne) bIsCrouched ? UnCrouch() : Crouch();
}

void AGun_phiriaCharacter::InputLean(const FInputActionValue& Value) { TargetLean = Value.Get<float>(); }
void AGun_phiriaCharacter::InputLeanEnd(const FInputActionValue& Value) { TargetLean = 0.0f; }

void AGun_phiriaCharacter::ToggleProne()
{
	if (bIsCasting)
	{
		CancelCasting();
		return;
	}

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

void AGun_phiriaCharacter::CustomJump()
{
	// 아이템 시전 중 점프를 시도하면 시전 취소
	if (bIsCasting)
	{
		CancelCasting();
		return;
	}

	if (bIsChangingStance) return;
	if (bIsProne) ToggleProne();
	else if (bIsCrouched) UnCrouch();
	else Jump();
}

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
	// --- 수정된 부분: 타겟이 이전과 달라졌을 때만 상태를 업데이트하고 이벤트를 호출합니다. ---
	if (TargetInteractable != BestTarget)
	{
		TargetInteractable = BestTarget;
	}
}

void AGun_phiriaCharacter::Interact()
{
	if (TargetInteractable && TargetInteractable->Implements<UInteractInterface>())
	{
		IInteractInterface::Execute_Interact(TargetInteractable.Get(), this);

		// 아이템을 주운 직후, 인벤토리가 열려있다면 UI를 새로고침합니다.
		if (bIsInventoryOpen && InventoryWidgetInstance)
		{
			// 블루프린트의 RefreshInventory(우리가 이름을 바꾼 RefreshBackpack) 함수 호출
			UFunction* RefreshFunc = InventoryWidgetInstance->FindFunction(FName("RefreshInventory"));
			if (RefreshFunc)
			{
				InventoryWidgetInstance->ProcessEvent(RefreshFunc, nullptr);
			}
		}
	}
}

// ==========================================
// --- Currency System Implementation ---
// ==========================================

void AGun_phiriaCharacter::AddGold(int32 Amount)
{
	// 음수 값이 들어오는 것을 방지합니다.
	if (Amount > 0)
	{
		CurrentGold += Amount;
		// 디버그용 출력 (나중에 지워도 됩니다)
		//GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, FString::Printf(TEXT("골드 획득! 현재 골드: %d"), CurrentGold));
	}
}

bool AGun_phiriaCharacter::SpendGold(int32 Amount)
{
	// 현재 골드가 요구량보다 많거나 같을 때만 소비 성공
	if (Amount > 0 && CurrentGold >= Amount)
	{
		CurrentGold -= Amount;
		return true; // 구매 성공
	}
	return false; // 구매 실패 (돈 부족)
}

void AGun_phiriaCharacter::ResetGold()
{
	CurrentGold = 0;
	//GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("마을 귀환: 골드가 초기화되었습니다."));
}

void AGun_phiriaCharacter::AddSapphire(int32 Amount)
{
	if (Amount > 0)
	{
		CurrentSapphire += Amount;
		//GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, FString::Printf(TEXT("사파이어 획득! 현재 사파이어: %d"), CurrentSapphire));
	}
}

bool AGun_phiriaCharacter::SpendSapphire(int32 Amount)
{
	if (Amount > 0 && CurrentSapphire >= Amount)
	{
		CurrentSapphire -= Amount;
		return true;
	}
	return false;
}

// 치트 명령어 실행 함수
void AGun_phiriaCharacter::CheatCurrency(int32 GoldAmount, int32 SapphireAmount)
{
	AddGold(GoldAmount);
	AddSapphire(SapphireAmount);
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("[치트 발동] 골드 +%d, 사파이어 +%d"), GoldAmount, SapphireAmount));
}

void AGun_phiriaCharacter::ForceBlackScreen()
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (PC->PlayerCameraManager)
		{
			// (시작 투명도 1=검은색, 끝 투명도 1, 시간 0=즉시, 색상, 오디오 유지, 페이드 유지)
			PC->PlayerCameraManager->StartCameraFade(1.0f, 1.0f, 0.0f, FLinearColor::Black, false, true);
		}
	}
}

void AGun_phiriaCharacter::StartFadeIn(float FadeInDuration)
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (PC->PlayerCameraManager)
		{
			// (시작 투명도 1=검은색, 끝 투명도 0=투명, 지정한 시간동안 서서히 밝아짐)
			PC->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, FadeInDuration, FLinearColor::Black, false, false);
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
		InventoryWidgetInstance->SetVisibility(ESlateVisibility::Visible);

		UFunction* RefreshFunc = InventoryWidgetInstance->FindFunction(FName("RefreshInventory"));
		if (RefreshFunc) InventoryWidgetInstance->ProcessEvent(RefreshFunc, nullptr);

		if (UInventoryMainWidget* MainWidget = Cast<UInventoryMainWidget>(InventoryWidgetInstance))
		{
			// 키보드로 직접 열었을 때는 항상 '일반 파밍 모드'로 열리도록 보장
			MainWidget->CurrentMode = EInventoryMode::IM_Normal;

			// [수정됨] 위젯의 공개 함수를 호출하여 타이머 시작!
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

		if (UInventoryMainWidget* MainWidget = Cast<UInventoryMainWidget>(InventoryWidgetInstance))
		{
			// [수정됨] 위젯의 공개 함수를 호출하여 타이머 정지!
			MainWidget->StopNearbyTimer();
		}

		PC->SetShowMouseCursor(false);
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
	}
}

TArray<APickupItemBase*> AGun_phiriaCharacter::GetNearbyItems()
{
	TArray<APickupItemBase*> NearbyItems;
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	// 내 캐릭터 반경 200 유닛 안의 아이템 스캔
	if (GetWorld()->OverlapMultiByChannel(Overlaps, GetActorLocation(), FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(200.f), Params))
	{
		for (const auto& Res : Overlaps)
		{
			// 겹친 게 PickupItemBase인지 캐스팅
			if (APickupItemBase* FoundItem = Cast<APickupItemBase>(Res.GetActor()))
			{
				//NearbyItems.Add(FoundItem);
				NearbyItems.AddUnique(FoundItem);
			}
		}
	}
	return NearbyItems;
}

// --- StartCasting 함수 ---
void AGun_phiriaCharacter::StartCasting(float Duration, FName ItemID, TFunction<void()> OnSuccess)
{
	if (bIsCasting) CancelCasting();

	bIsCasting = true;
	OnCastSuccessCallback = OnSuccess;

	OriginalWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	GetCharacterMovement()->MaxWalkSpeed = CastingWalkSpeed;

	// ==========================================
	// 1. 데이터 테이블에서 아이템 아이콘 텍스처 가져오기
	// ==========================================
	UTexture2D* IconTexture = nullptr;
	if (PlayerInventory && PlayerInventory->ItemDataTable)
	{
		// [주의] FItemData 구조체 안에 UTexture2D* ItemIcon 변수가 있다고 가정한 코드입니다. 변수명이 다르면 맞춰주세요!
		FItemData* ItemInfo = PlayerInventory->ItemDataTable->FindRow<FItemData>(ItemID, TEXT("GetIcon"));
		if (ItemInfo)
		{
			IconTexture = ItemInfo->ItemIcon;
		}
	}

	// ==========================================
	// 2. C++ 위젯 생성 및 화면에 띄우기 (안전한 PlayerController 방식)
	// ==========================================
	if (CastBarWidgetClass)
	{
		if (!CastBarInstance)
		{
			// GetWorld() 대신 플레이어 컨트롤러를 명확하게 넘겨줍니다!
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
			// C++ 위젯의 함수 직접 호출 (타이머 시작, 아이콘 적용)
			CastBarInstance->StartCast(Duration, IconTexture);
		}
	}

	// ==========================================
	// 3. 타이머 로직
	// ==========================================
	GetWorldTimerManager().SetTimer(CastTimerHandle, [this]() {
		bIsCasting = false;
		GetCharacterMovement()->MaxWalkSpeed = OriginalWalkSpeed;

		// 캐스팅 완료 시 화면에서 UI 제거
		if (CastBarInstance && CastBarInstance->IsInViewport())
		{
			CastBarInstance->RemoveFromParent();
		}

		if (OnCastSuccessCallback) OnCastSuccessCallback();
		}, Duration, false);
}

void AGun_phiriaCharacter::CancelCasting()
{
	if (!bIsCasting) return;

	bIsCasting = false;
	GetWorldTimerManager().ClearTimer(CastTimerHandle);
	GetCharacterMovement()->MaxWalkSpeed = OriginalWalkSpeed;

	// 취소 시 화면에서 UI 제거
	if (CastBarInstance && CastBarInstance->IsInViewport())
	{
		CastBarInstance->RemoveFromParent();
	}
}

void AGun_phiriaCharacter::ApplySpeedBuff(float BoostAmount, float Duration)
{
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp) return;

	// 1. 이미 버프가 걸려있다면? 기존 버프 수치를 빼고 타이머를 정지시킵니다.
	if (bHasSpeedBuff)
	{
		GetWorldTimerManager().ClearTimer(SpeedBuffTimerHandle);
		if (bIsCasting) OriginalWalkSpeed -= CurrentSpeedBoost;
		else MoveComp->MaxWalkSpeed -= CurrentSpeedBoost;
	}

	// 2. 새로운 버프 적용 및 상태 갱신
	bHasSpeedBuff = true;
	CurrentSpeedBoost = BoostAmount;

	if (bIsCasting) OriginalWalkSpeed += CurrentSpeedBoost;
	else MoveComp->MaxWalkSpeed += CurrentSpeedBoost;

	// 3. 타이머 새로 시작 (Duration 초 뒤에 원래 속도로 복구)
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
	// 1. 기존에 돌고 있던 회복 타이머가 있다면 즉시 정지 (초기화)
	GetWorldTimerManager().ClearTimer(HOTTimerHandle);

	// 2. 새로운 틱 계산
	MaxHOTTicks = FMath::FloorToInt(Duration);
	CurrentHOTTicks = 0;
	HOTAmountPerTick = TotalHeal / Duration;

	// 3. 1초마다 반복되는 타이머 새로 시작
	TWeakObjectPtr<AGun_phiriaCharacter> WeakThis(this);
	GetWorldTimerManager().SetTimer(HOTTimerHandle, [WeakThis]()
		{
			if (WeakThis.IsValid())
			{
				// 체력 회복
				WeakThis->CurrentHealth = FMath::Clamp(WeakThis->CurrentHealth + WeakThis->HOTAmountPerTick, 0.0f, WeakThis->MaxHealth);
				WeakThis->CurrentHOTTicks++;

				// 목표 횟수에 도달하면 타이머 종료
				if (WeakThis->CurrentHOTTicks >= WeakThis->MaxHOTTicks)
				{
					WeakThis->GetWorldTimerManager().ClearTimer(WeakThis->HOTTimerHandle);
				}
			}
		}, 1.0f, true);
}

void AGun_phiriaCharacter::UpdateEquipmentVisuals(EEquipType EquipType, UStaticMesh* NewMesh)
{
	switch (EquipType)
	{
	case EEquipType::Helmet:
		if (HelmetMesh) HelmetMesh->SetStaticMesh(NewMesh);
		if (CloneHelmetMesh) CloneHelmetMesh->SetStaticMesh(NewMesh);
		break;

	case EEquipType::Vest:
		if (VestMesh) VestMesh->SetStaticMesh(NewMesh);
		if (CloneVestMesh) CloneVestMesh->SetStaticMesh(NewMesh);
		break;

	case EEquipType::Backpack:
		if (BackpackMesh) BackpackMesh->SetStaticMesh(NewMesh);
		if (CloneBackpackMesh) CloneBackpackMesh->SetStaticMesh(NewMesh);
		break;

	default:
		break;
	}
}

void AGun_phiriaCharacter::DropItemToGround(FName ItemID)
{
	if (!PlayerInventory || !PlayerInventory->ItemDataTable) return;

	// 1. 인벤토리에서 해당 아이템 1개 제거 (무게 감소까지 완벽하게 처리됨!)
	bool bRemoved = PlayerInventory->RemoveItem(ItemID, 1);
	if (!bRemoved) return;

	// 2. 데이터 테이블에서 스폰할 3D 아이템 클래스 가져오기
	FItemData* ItemData = PlayerInventory->ItemDataTable->FindRow<FItemData>(ItemID, TEXT("DropItem"));

	// [중요] FItemData 구조체 안에 TSubclassOf<APickupItemBase> ItemClass 변수가 존재해야 합니다!
	if (ItemData && ItemData->ItemClass)
	{
		// 3. 내 캐릭터 앞쪽 바닥 위치 계산
		FVector SpawnLoc = GetActorLocation() + (GetActorForwardVector() * 100.0f);
		SpawnLoc.Z -= 80.0f; // 캡슐 바닥 쪽으로 내림

		// 4. 월드에 아이템 액터 스폰
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

void AGun_phiriaCharacter::RestoreEquipmentVisuals()
{
	// 인벤토리와 데이터 테이블이 유효한지 확인
	if (!PlayerInventory || !PlayerInventory->ItemDataTable) return;

	// 중복 코드를 줄이기 위한 람다(Lambda) 함수
	auto ApplyEquipMesh = [&](FName ItemID, EEquipType EquipType, const TCHAR* Context)
		{
			if (!ItemID.IsNone())
			{
				// 데이터 테이블에서 아이템 정보를 찾습니다.
				if (FItemData* ItemData = PlayerInventory->ItemDataTable->FindRow<FItemData>(ItemID, Context))
				{
					// =========================================================
					// [주의] ItemData->ItemMesh 부분은 네가 만든 구조체(FItemData) 안에 있는 
					// 실제 '스태틱 메시 변수명'으로 꼭 바꿔 적어줘야 해!! (예: EquipMesh 등)
					// =========================================================
					UpdateEquipmentVisuals(EquipType, ItemData->EquipmentMesh);
				}
			}
		};

	// 헬멧, 조끼, 가방 순서대로 복구 실행
	ApplyEquipMesh(PlayerInventory->EquippedHelmetID, EEquipType::Helmet, TEXT("RestoreHelmet"));
	ApplyEquipMesh(PlayerInventory->EquippedVestID, EEquipType::Vest, TEXT("RestoreVest"));
	ApplyEquipMesh(PlayerInventory->EquippedBackpackID, EEquipType::Backpack, TEXT("RestoreBackpack"));
}