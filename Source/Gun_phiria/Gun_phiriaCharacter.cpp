#include "Gun_phiriaCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "DrawDebugHelpers.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Engine/DamageEvents.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Interface/InteractInterface.h"
#include "Engine/OverlapResult.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

AGun_phiriaCharacter::AGun_phiriaCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	// 이동 세팅
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// 기본 카메라 세팅
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// 조준 카메라 세팅
	ADSCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ADSCamera"));
	ADSCamera->SetupAttachment(GetMesh());
	ADSCamera->bUsePawnControlRotation = false;
	ADSCamera->SetAutoActivate(false);
	ADSCamera->PrimaryComponentTick.TickGroup = TG_PostPhysics;

	// 콜리전 세팅
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	// 앉기 활성화
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
		GetCharacterMovement()->MaxWalkSpeedCrouched = 250.0f;
	}

	bIsProne = false;
	DefaultCapsuleHalfHeight = 96.0f;
	DefaultMeshRelativeLocationZ = -97.0f;
}

void AGun_phiriaCharacter::BeginPlay()
{
	Super::BeginPlay();

	GetWorldTimerManager().SetTimer(InteractionTimerHandle, this, &AGun_phiriaCharacter::CheckForInteractables, 0.1f, true);

	CurrentHealth = MaxHealth;

	// 기본 무기 스폰 및 장착
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

	if (ADSCamera && GetMesh())
	{
		ADSCamera->AddTickPrerequisiteComponent(GetMesh());
	}
}

void AGun_phiriaCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 카메라 FOV 보간
	if (FollowCamera && ADSCamera)
	{
		float TargetFOV = bIsAiming ? AimFOV : DefaultFOV;
		UCameraComponent* CurrentActiveCam = bIsAiming ? ADSCamera : FollowCamera;

		if (!FMath::IsNearlyEqual(CurrentActiveCam->FieldOfView, TargetFOV, 0.5f))
		{
			float NewFOV = FMath::FInterpTo(CurrentActiveCam->FieldOfView, TargetFOV, DeltaTime, ZoomInterpSpeed);
			CurrentActiveCam->SetFieldOfView(NewFOV);
		}
		else
		{
			CurrentActiveCam->SetFieldOfView(TargetFOV);
		}
	}

	// 상태별 기본 탄 퍼짐(Spread) 계산
	float CurrentSpeed = GetVelocity().Size2D();
	bool bIsFalling = GetCharacterMovement()->IsFalling();
	float TargetMinSpread = 0.0f;

	if (bIsFalling)
	{
		TargetMinSpread = bIsAiming ? 3.0f : 6.0f;
	}
	else if (CurrentSpeed > 10.0f)
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

	// 탄 퍼짐 회복
	if (GetWorld()->GetTimeSeconds() - LastFireTime > SpreadRecoveryDelay)
	{
		CurrentSpread = FMath::FInterpTo(CurrentSpread, TargetMinSpread, DeltaTime, SpreadRecoveryRate);
	}

	// 절차적 조준(Procedural ADS) 오프셋 연산
	float TargetAlpha = bIsAiming ? 1.0f : 0.0f;
	ADSAlpha = FMath::FInterpTo(ADSAlpha, TargetAlpha, DeltaTime, 15.0f);

	if (ADSAlpha > 0.01f && FollowCamera && CurrentWeapon && CurrentWeapon->GetWeaponMesh())
	{
		FVector CameraLocation = FollowCamera->GetComponentLocation();
		FVector CameraForward = FollowCamera->GetForwardVector();

		FVector CamToChar = GetActorLocation() - CameraLocation;
		float ActualDistance = FVector::DotProduct(CamToChar, CameraForward);
		float TotalDistance = ActualDistance + AimDistance;

		FVector TargetSightWorldLoc = CameraLocation + (CameraForward * TotalDistance);
		FVector HandWorldLoc = CurrentWeapon->GetWeaponMesh()->GetComponentLocation();
		FVector SightWorldLoc = CurrentWeapon->GetWeaponMesh()->GetSocketLocation(FName("SightSocket"));

		FVector TargetHandWorldLoc = TargetSightWorldLoc + (HandWorldLoc - SightWorldLoc);
		FTransform MeshTransform = GetMesh()->GetComponentTransform();

		DynamicAimOffset = FMath::Lerp(FVector::ZeroVector, MeshTransform.InverseTransformPosition(TargetHandWorldLoc), ADSAlpha);
	}
	else if (ADSAlpha <= 0.01f)
	{
		DynamicAimOffset = FVector::ZeroVector;
	}

	// 애니메이션용 로코모션 변수 계산 (Direction, YawSpeed)
	FVector Velocity = GetVelocity();
	if (Velocity.Size2D() > 1.0f)
	{
		FVector LocalVelocity = GetActorTransform().InverseTransformVectorNoScale(Velocity);
		MovementDirectionAngle = LocalVelocity.Rotation().Yaw;
	}
	else
	{
		MovementDirectionAngle = 0.0f;
	}

	float CurrentYaw = GetActorRotation().Yaw;
	float YawDelta = CurrentYaw - PreviousActorYaw;
	if (YawDelta > 180.0f) YawDelta -= 360.0f;
	if (YawDelta < -180.0f) YawDelta += 360.0f;

	YawRotationSpeed = YawDelta / DeltaTime;
	PreviousActorYaw = CurrentYaw;

	// 헤드샷 조준 판별
	bIsAimingAtHead = false;
	if (Controller != nullptr)
	{
		FVector CameraLocation;
		FRotator CameraRotation;
		Controller->GetPlayerViewPoint(CameraLocation, CameraRotation);

		FVector TraceEnd = CameraLocation + (CameraRotation.Vector() * 5000.0f);
		FHitResult HitResult;
		FCollisionQueryParams TraceParams;
		TraceParams.AddIgnoredActor(this);
		if (CurrentWeapon) TraceParams.AddIgnoredActor(CurrentWeapon);

		if (GetWorld()->LineTraceSingleByChannel(HitResult, CameraLocation, TraceEnd, ECC_Visibility, TraceParams))
		{
			if (HitResult.GetActor() && Cast<ACharacter>(HitResult.GetActor()))
			{
				if (HitResult.BoneName == FName("head")) bIsAimingAtHead = true;
			}
		}
	}

	// 기울기(Lean) 적용
	CurrentLean = FMath::FInterpTo(CurrentLean, TargetLean, DeltaTime, 10.0f);
	if (FollowCamera && ADSCamera && GetMesh())
	{
		LeanAxisCS = GetMesh()->GetComponentTransform().InverseTransformVectorNoScale(GetBaseAimRotation().Vector());
	}

	// 동적 카메라 높이 및 좌우 시야 확보
	if (CameraBoom)
	{
		float TargetHeight = 120.0f;
		if (bIsProne) TargetHeight = 60.0f;
		else if (bIsCrouched) TargetHeight = 90.0f;

		FVector TargetOffset = FVector(0.0f, CurrentLean * 100.0f, TargetHeight);
		CameraBoom->SocketOffset = FMath::VInterpTo(CameraBoom->SocketOffset, TargetOffset, DeltaTime, 10.0f);
	}

	// 총기 벽 뚫림 방지 (밀어내기)
	if (CurrentWeapon && CurrentWeapon->GetWeaponMesh())
	{
		FVector MuzzleLoc = CurrentWeapon->GetWeaponMesh()->GetSocketLocation(FName("MuzzleSocket"));
		FVector Start = GetActorLocation();
		Start.Z = MuzzleLoc.Z;

		FVector Direction = (MuzzleLoc - Start).GetSafeNormal();
		FVector End = MuzzleLoc + (Direction * 15.0f);

		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);
		Params.AddIgnoredActor(CurrentWeapon);

		if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
		{
			if (FMath::Abs(Hit.ImpactNormal.Z) < 0.3f)
			{
				float PenetrationDepth = FVector::Distance(Hit.ImpactPoint, End);
				FVector PushDirection = Hit.ImpactNormal;
				PushDirection.Z = 0.0f;
				PushDirection.Normalize();

				AddActorWorldOffset(PushDirection * PenetrationDepth, true);
			}
		}
	}
}

// Input 설정
void AGun_phiriaCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (JumpAction)
		{
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AGun_phiriaCharacter::CustomJump);
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		}
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AGun_phiriaCharacter::Move);
		}
		if (LookAction)
		{
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AGun_phiriaCharacter::Look);
		}
		if (AimAction)
		{
			EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &AGun_phiriaCharacter::StartAiming);
			EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &AGun_phiriaCharacter::StopAiming);
		}
		if (FireAction)
		{
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AGun_phiriaCharacter::Fire);
		}
		if (CrouchAction)
		{
			EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &AGun_phiriaCharacter::ToggleCrouch);
		}
		if (LeanAction)
		{
			EnhancedInputComponent->BindAction(LeanAction, ETriggerEvent::Triggered, this, &AGun_phiriaCharacter::InputLean);
			EnhancedInputComponent->BindAction(LeanAction, ETriggerEvent::Completed, this, &AGun_phiriaCharacter::InputLeanEnd);
		}
		if (ProneAction)
		{
			EnhancedInputComponent->BindAction(ProneAction, ETriggerEvent::Started, this, &AGun_phiriaCharacter::ToggleProne);
		}
		if (InteractAction)
		{
			EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AGun_phiriaCharacter::Interact);
		}
	}
}

void AGun_phiriaCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		float ForwardInput = MovementVector.Y;

		// 총구 벽 뚫림 방지 (전진 차단)
		if (CurrentWeapon && CurrentWeapon->GetWeaponMesh() && ForwardInput > 0.0f)
		{
			FVector MuzzleLocation = CurrentWeapon->GetWeaponMesh()->GetSocketLocation(FName("MuzzleSocket"));
			FVector TraceStart = MuzzleLocation - (ForwardDirection * 20.0f);
			FVector TraceEnd = MuzzleLocation + (ForwardDirection * 15.0f);

			FHitResult Hit;
			FCollisionQueryParams Params;
			Params.AddIgnoredActor(this);
			Params.AddIgnoredActor(CurrentWeapon);

			FCollisionShape Sphere = FCollisionShape::MakeSphere(10.0f);

			if (GetWorld()->SweepSingleByChannel(Hit, TraceStart, TraceEnd, FQuat::Identity, ECC_Visibility, Sphere, Params))
			{
				ForwardInput = 0.0f;
				// DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 10.0f, 12, FColor::Red, false, 0.1f); // 디버그용 주석 처리
			}
		}

		AddMovementInput(ForwardDirection, ForwardInput);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AGun_phiriaCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AGun_phiriaCharacter::StartAiming()
{
	bIsAiming = true;
	if (FollowCamera && ADSCamera)
	{
		FollowCamera->SetActive(false);
		ADSCamera->SetActive(true);
	}
}

void AGun_phiriaCharacter::StopAiming()
{
	bIsAiming = false;
	if (FollowCamera && ADSCamera)
	{
		ADSCamera->SetActive(false);
		FollowCamera->SetActive(true);
	}
}

void AGun_phiriaCharacter::Fire()
{
	if (!CurrentWeapon || !FollowCamera || !ADSCamera) return;

	bIsFiring = true;
	GetWorldTimerManager().SetTimer(FireTimerHandle, this, &AGun_phiriaCharacter::StopFiringPose, 1.0f, false);
	LastFireTime = GetWorld()->GetTimeSeconds();

	// 사격 애니메이션 재생
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		TArray<UAnimMontage*> MontagesToPlay = bIsProne ? CurrentWeapon->ProneFireMontages : CurrentWeapon->StandCrouchFireMontages;

		if (MontagesToPlay.Num() > 0)
		{
			int32 RandomIndex = FMath::RandRange(0, MontagesToPlay.Num() - 1);
			AnimInstance->Montage_Play(MontagesToPlay[RandomIndex]);
		}
	}

	// 반동 및 탄 퍼짐 계산
	float AimMultiplier = bIsAiming ? 0.6f : 1.2f;
	float MovementMultiplier = (GetVelocity().Size2D() > 10.0f) ? 1.5f : 1.0f;
	float FallMultiplier = GetCharacterMovement()->IsFalling() ? 2.5f : 1.0f;

	float StanceMultiplier = 1.0f;
	if (bIsProne) StanceMultiplier = 0.4f;
	else if (bIsCrouched) StanceMultiplier = 0.75f;

	float TotalMultiplier = AimMultiplier * MovementMultiplier * FallMultiplier * StanceMultiplier;

	if (Controller != nullptr)
	{
		AddControllerPitchInput(FMath::RandRange(-0.5f, -1.0f) * TotalMultiplier);
		AddControllerYawInput(FMath::RandRange(-0.5f, 0.5f) * TotalMultiplier);
	}

	CurrentSpread = FMath::Clamp(CurrentSpread + (SpreadPerShot * TotalMultiplier), 0.0f, MaxSpread);

	// 탄도 계산 및 사격
	UCameraComponent* ActiveCamera = bIsAiming ? ADSCamera : FollowCamera;
	FVector CameraLocation = ActiveCamera->GetComponentLocation();
	FVector CameraForward = ActiveCamera->GetForwardVector();
	FVector CameraEndLocation = CameraLocation + (CameraForward * 10000.0f);

	FHitResult CameraHit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	if (CurrentWeapon) QueryParams.AddIgnoredActor(CurrentWeapon);

	FVector EyeTargetLocation = GetWorld()->LineTraceSingleByChannel(CameraHit, CameraLocation, CameraEndLocation, ECC_Visibility, QueryParams)
		? CameraHit.ImpactPoint : CameraEndLocation;

	FVector MuzzleLocation = CurrentWeapon->GetWeaponMesh()->GetSocketLocation(FName("MuzzleSocket"));
	FVector RealFireDirection = (EyeTargetLocation - MuzzleLocation).GetSafeNormal();
	FVector FinalTraceEnd = MuzzleLocation + (RealFireDirection * 5000.0f);

	FHitResult FinalHit;
	FVector FinalTargetLocation = GetWorld()->LineTraceSingleByChannel(FinalHit, MuzzleLocation, FinalTraceEnd, ECC_Visibility, QueryParams)
		? FinalHit.ImpactPoint : FinalTraceEnd;

	float SpreadMultiplier = CurrentWeapon->WeaponSpreadMultiplier;
	float FinalSpreadAngle = CurrentSpread * SpreadMultiplier;

	FVector Right, Up;
	RealFireDirection.FindBestAxisVectors(Right, Up);

	float ActualDistanceToTarget = FVector::Distance(MuzzleLocation, FinalTargetLocation);
	float SpreadRadius = FMath::Tan(FMath::DegreesToRadians(FinalSpreadAngle)) * ActualDistanceToTarget;

	float RandomAngle = FMath::RandRange(0.0f, PI * 2.0f);
	float RandomRadius = FMath::RandRange(0.0f, SpreadRadius);

	FVector SpreadOffset = (Right * FMath::Cos(RandomAngle) * RandomRadius) + (Up * FMath::Sin(RandomAngle) * RandomRadius);
	FVector TargetLocation = FinalTargetLocation + SpreadOffset;

	// GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan, FString::Printf(TEXT("Spread Multiplier: %.1f | Spread Angle: %.2f deg"), SpreadMultiplier, FinalSpreadAngle)); // 디버그용 주석 처리

	CurrentWeapon->Fire(TargetLocation);
}

void AGun_phiriaCharacter::StopFiringPose()
{
	bIsFiring = false;
}

float AGun_phiriaCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead) return 0.0f;

	float ActualDamage = DamageAmount;

	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FPointDamageEvent* PointDamageEvent = static_cast<const FPointDamageEvent*>(&DamageEvent);
		if (PointDamageEvent->HitInfo.BoneName == FName("head"))
		{
			ActualDamage *= 2.5f;
		}
	}

	ActualDamage = Super::TakeDamage(ActualDamage, DamageEvent, EventInstigator, DamageCauser);

	// 피격 애니메이션 재생
	if (HitMontages.Num() > 0)
	{
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			AnimInstance->Montage_Play(HitMontages[FMath::RandRange(0, HitMontages.Num() - 1)]);
		}
	}

	CurrentHealth = FMath::Clamp(CurrentHealth - ActualDamage, 0.0f, MaxHealth);

	if (CurrentHealth <= 0.0f)
	{
		Die();
	}

	return ActualDamage;
}

void AGun_phiriaCharacter::Die()
{
	if (bIsDead) return;
	bIsDead = true;

	DisableInput(Cast<APlayerController>(GetController()));
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	GetMesh()->SetSimulatePhysics(true);

	// GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("YOU DIED")); // 디버그용 주석 처리
}

void AGun_phiriaCharacter::ToggleCrouch()
{
	if (bIsProne) return;

	if (bIsCrouched) UnCrouch();
	else Crouch();
}

void AGun_phiriaCharacter::InputLean(const FInputActionValue& Value)
{
	TargetLean = Value.Get<float>();
}

void AGun_phiriaCharacter::InputLeanEnd(const FInputActionValue& Value)
{
	TargetLean = 0.0f;
}

void AGun_phiriaCharacter::ToggleProne()
{
	if (GetCharacterMovement()->IsFalling()) return;

	if (bIsProne)
	{
		bIsProne = false;
		GetCapsuleComponent()->SetCapsuleHalfHeight(DefaultCapsuleHalfHeight);

		FVector MeshLoc = GetMesh()->GetRelativeLocation();
		MeshLoc.Z = DefaultMeshRelativeLocationZ;
		GetMesh()->SetRelativeLocation(MeshLoc);

		GetCharacterMovement()->MaxWalkSpeed = 500.0f;
	}
	else
	{
		if (bIsCrouched) UnCrouch();

		bIsProne = true;
		float ProneHalfHeight = 34.0f;
		GetCapsuleComponent()->SetCapsuleHalfHeight(ProneHalfHeight);

		FVector MeshLoc = GetMesh()->GetRelativeLocation();
		MeshLoc.Z = DefaultMeshRelativeLocationZ + (DefaultCapsuleHalfHeight - ProneHalfHeight);
		GetMesh()->SetRelativeLocation(MeshLoc);

		GetCharacterMovement()->MaxWalkSpeed = MaxWalkSpeedProne;
	}
}

void AGun_phiriaCharacter::CustomJump()
{
	if (bIsChangingStance) return;

	if (bIsProne) ToggleProne();
	else if (bIsCrouched) UnCrouch();
	else ACharacter::Jump();
}

void AGun_phiriaCharacter::CheckForInteractables()
{
	float ScanRange = 200.0f; // 상호작용 가능 거리
	TArray<FOverlapResult> OverlapResults;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(ScanRange);
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	// 주변 스캔 (성능을 위해 Visibility 채널 사용)
	bool bHit = GetWorld()->OverlapMultiByChannel(OverlapResults, GetActorLocation(), FQuat::Identity, ECC_Visibility, Sphere, Params);

	AActor* BestTarget = nullptr;
	float MinDistance = ScanRange;

	if (bHit)
	{
		for (const FOverlapResult& Result : OverlapResults)
		{
			AActor* Potential = Result.GetActor();
			// 인터페이스를 가지고 있는지 확인
			if (Potential && Potential->Implements<UInteractInterface>())
			{
				float Dist = FVector::Dist(GetActorLocation(), Potential->GetActorLocation());
				if (Dist < MinDistance)
				{
					MinDistance = Dist;
					BestTarget = Potential;
				}
			}
		}
	}

	// 타겟이 바뀌었을 때만 갱신 (HUD 업데이트 최적화를 위해)
	if (BestTarget != TargetInteractable)
	{
		TargetInteractable = BestTarget;

		// 여기에 나중에 블루프린트로 UI를 끄고 켜는 이벤트를 보낼 수 있어!
	}
}

void AGun_phiriaCharacter::Interact()
{
	// 타겟이 유효하고 인터페이스를 가지고 있다면 실행
	if (TargetInteractable && TargetInteractable->Implements<UInteractInterface>())
	{
		// 1. 애니메이션 판단 (인터페이스의 ShouldPlayAnimation 함수 호출)
		if (IInteractInterface::Execute_ShouldPlayAnimation(TargetInteractable) && InteractMontage)
		{
			if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
			{
				AnimInstance->Montage_Play(InteractMontage);
			}
		}

		// 2. 실제 상호작용 로직 실행 (아이템 줍기 등)
		IInteractInterface::Execute_Interact(TargetInteractable, this);
	}
}