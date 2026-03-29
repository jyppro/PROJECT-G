#include "Gun_phiriaCharacter.h"
#include "Weapon/WeaponBase.h"
#include "Interface/InteractInterface.h"
#include "component/InventoryComponent.h"
#include "Blueprint/UserWidget.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Item/PickupItemBase.h"
#include "UI/InventoryMainWidget.h"

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
}

void AGun_phiriaCharacter::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;

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
		}
	}

	if (ADSCamera) ADSCamera->AddTickPrerequisiteComponent(GetMesh());

	// --- 인벤토리 UI 생성 및 숨김 처리 ---
	if (InventoryWidgetClass)
	{
		InventoryWidgetInstance = CreateWidget<UUserWidget>(GetWorld(), InventoryWidgetClass);
		if (InventoryWidgetInstance)
		{
			InventoryWidgetInstance->AddToViewport();
			InventoryWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (InventoryCamera)
	{
		InventoryCamera->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
		InventoryCamera->ShowOnlyComponent(InventoryCloneMesh);
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
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		if (static_cast<const FPointDamageEvent*>(&DamageEvent)->HitInfo.BoneName == FName("head")) ActualDamage *= 2.5f;
	}

	ActualDamage = Super::TakeDamage(ActualDamage, DamageEvent, EventInstigator, DamageCauser);
	if (HitMontages.Num() > 0) PlayAnimMontage(HitMontages[FMath::RandRange(0, HitMontages.Num() - 1)]);

	CurrentHealth = FMath::Clamp(CurrentHealth - ActualDamage, 0.0f, MaxHealth);
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

void AGun_phiriaCharacter::ToggleCrouch() { if (!bIsProne) bIsCrouched ? UnCrouch() : Crouch(); }
void AGun_phiriaCharacter::InputLean(const FInputActionValue& Value) { TargetLean = Value.Get<float>(); }
void AGun_phiriaCharacter::InputLeanEnd(const FInputActionValue& Value) { TargetLean = 0.0f; }

void AGun_phiriaCharacter::ToggleProne()
{
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
		float MinDist = 200.f;
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
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, FString::Printf(TEXT("골드 획득! 현재 골드: %d"), CurrentGold));
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
	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("마을 귀환: 골드가 초기화되었습니다."));
}

void AGun_phiriaCharacter::AddSapphire(int32 Amount)
{
	if (Amount > 0)
	{
		CurrentSapphire += Amount;
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, FString::Printf(TEXT("사파이어 획득! 현재 사파이어: %d"), CurrentSapphire));
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
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("[치트 발동] 골드 +%d, 사파이어 +%d"), GoldAmount, SapphireAmount));
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
	// 위젯이 생성되지 않았거나 플레이어 컨트롤러가 없으면 중단
	if (!InventoryWidgetInstance) return;

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	// 상태 뒤집기 (열려있으면 닫고, 닫혀있으면 열기)
	bIsInventoryOpen = !bIsInventoryOpen;

	if (bIsInventoryOpen)
	{
		// 1. UI 보이기
		InventoryWidgetInstance->SetVisibility(ESlateVisibility::Visible);

		// 블루프린트 이벤트 호출 (가방 업데이트)
		UFunction* RefreshFunc = InventoryWidgetInstance->FindFunction(FName("RefreshInventory"));
		if (RefreshFunc) InventoryWidgetInstance->ProcessEvent(RefreshFunc, nullptr);

		// 방금 만든 C++ 강제 새로고침 호출! (주변 아이템 업데이트)
		if (UInventoryMainWidget* MainWidget = Cast<UInventoryMainWidget>(InventoryWidgetInstance))
		{
			MainWidget->ForceNearbyRefresh();
		}

		// 3. 마우스 켜기 및 UI 조작 모드로 전환
		PC->SetShowMouseCursor(true);
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(InventoryWidgetInstance->TakeWidget());
		PC->SetInputMode(InputMode);
	}
	else
	{
		// 1. UI 숨기기
		InventoryWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);

		// 2. 마우스 끄기 및 게임 전용 모드로 복귀
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