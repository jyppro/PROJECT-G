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
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"


DEFINE_LOG_CATEGORY(LogTemplateCharacter);

AGun_phiriaCharacter::AGun_phiriaCharacter()
{
	// Tick 함수 작동 - 카메라 스무스 줌
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	// 캐릭터가 입력 방향을 바라보며 뛰어가는 기능 끄기 (게걸음, 뒷걸음질 활성화)
	GetCharacterMovement()->bOrientRotationToMovement = false;

	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// --- 조준 전용 카메라 생성 및 가늠좌 소켓에 부착 ---
	ADSCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ADSCamera"));

	// 카메라는 마우스가 아닌 '총의 애니메이션 흔들림'을 그대로 따라가야 하므로 false로 설정
	ADSCamera->bUsePawnControlRotation = false;

	// 평소(비조준 상태)에는 꺼져 있어야 하므로 기본 활성화 상태를 false로 둠
	ADSCamera->SetAutoActivate(false);

	// ★ [콜리전 자동화] 블루프린트에서 하던 설정을 C++로 고정!

	// 1. 캡슐 컴포넌트는 총알(Visibility)을 무시(Ignore)하게 만듭니다.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);

	// 2. 스켈레탈 메시는 충돌 연산을 켜주고, 총알(Visibility)을 막아내게(Block) 만듭니다.
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}

void AGun_phiriaCharacter::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth; // 시작 시 체력 100

	// 게임 시작 시 무기 스폰 및 장착
	if (DefaultWeaponClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		CurrentWeapon = GetWorld()->SpawnActor<AWeaponBase>(DefaultWeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

		if (CurrentWeapon)
		{
			FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
			CurrentWeapon->AttachToComponent(GetMesh(), AttachmentRules, FName("WeaponSocket"));

			// 기존의 ADSCamera 부착 로직을 무기가 스폰된 직후로 이동
			ADSCamera->AttachToComponent(CurrentWeapon->GetWeaponMesh(), AttachmentRules, FName("SightSocket"));
		}
	}
}

void AGun_phiriaCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 카메라 이동 로직
	if (FollowCamera && ADSCamera)
	{
		float TargetFOV = bIsAiming ? AimFOV : DefaultFOV;
		UCameraComponent* CurrentActiveCam = bIsAiming ? ADSCamera : FollowCamera;

		// 현재 FOV와 목표 FOV의 차이가 작을 때만 업데이트
		if (!FMath::IsNearlyEqual(CurrentActiveCam->FieldOfView, TargetFOV, 0.5f))
		{
			float NewFOV = FMath::FInterpTo(CurrentActiveCam->FieldOfView, TargetFOV, DeltaTime, ZoomInterpSpeed);
			CurrentActiveCam->SetFieldOfView(NewFOV);
		}
		else
		{
			// 목표값에 완전히 고정하여 불필요한 연산 방지
			CurrentActiveCam->SetFieldOfView(TargetFOV);
		}
	}

	// 플레이어 상태(이동, 점프)에 따른 탄 퍼짐 기본값 설정
	float CurrentSpeed = GetVelocity().Size2D();
	bool bIsFalling = GetCharacterMovement()->IsFalling(); // 공중에 떠 있는지 확인

	float TargetMinSpread = 0.0f; // 기본 수치

	if (bIsFalling)
	{
		// 공중에 떠 있을 때 (가장 큰 페널티)
		TargetMinSpread = bIsAiming ? 3.0f : 6.0f;
	}
	else if (CurrentSpeed > 10.0f)
	{
		// 땅에 있지만 걷거나 뛸 때
		TargetMinSpread = bIsAiming ? 1.5f : 3.0f;
	}
	else
	{
		// 땅에서 가만히 서 있을 때
		TargetMinSpread = bIsAiming ? 0.0f : 1.0f; // 서 있을 때도 비조준 시 약간의 퍼짐 유지
	}

	float CurrentTime = GetWorld()->GetTimeSeconds(); // 현재 게임 플레이 시간 가져오기

	if (CurrentTime - LastFireTime > SpreadRecoveryDelay)
	{
		// 사격한 지 SpreadRecoveryDelay(예: 0.2초)가 지났을 때만 서서히 회복
		CurrentSpread = FMath::FInterpTo(CurrentSpread, TargetMinSpread, DeltaTime, SpreadRecoveryRate);
	}

	// --- 절차적 조준(Procedural ADS) 뼈대 타겟 좌표 계산 ---
	// 조건문에 CurrentWeapon이 유효한지 확인하는 로직 추가
	if (bIsAiming && FollowCamera && CurrentWeapon && CurrentWeapon->GetWeaponMesh())
	{
		FVector CameraLocation = FollowCamera->GetComponentLocation();
		FVector CameraForward = FollowCamera->GetForwardVector();

		FVector CamToChar = GetActorLocation() - CameraLocation;
		float ActualDistance = FVector::DotProduct(CamToChar, CameraForward);
		float TotalDistance = ActualDistance + AimDistance;

		FVector TargetSightWorldLoc = CameraLocation + (CameraForward * TotalDistance);

		// WeaponMesh 대신 CurrentWeapon->GetWeaponMesh()를 사용합니다!
		FVector HandWorldLoc = CurrentWeapon->GetWeaponMesh()->GetComponentLocation();
		FVector SightWorldLoc = CurrentWeapon->GetWeaponMesh()->GetSocketLocation(FName("SightSocket"));
		FVector SightToHandOffset = HandWorldLoc - SightWorldLoc;

		FVector TargetHandWorldLoc = TargetSightWorldLoc + SightToHandOffset;

		FTransform MeshTransform = GetMesh()->GetComponentTransform();
		FVector TargetHandCS = MeshTransform.InverseTransformPosition(TargetHandWorldLoc);

		DynamicAimOffset = FMath::VInterpTo(DynamicAimOffset, TargetHandCS, DeltaTime, 20.0f);
	}

	// 대각선 이동 애니메이션을 위한 '이동 방향 각도(Direction)' 계산
	FVector Velocity = GetVelocity();
	if (Velocity.Size2D() > 1.0f) // 캐릭터가 조금이라도 움직이고 있다면
	{
		// 월드 기준의 속도 벡터를 캐릭터 기준(Local) 벡터로 변환
		FVector LocalVelocity = GetActorTransform().InverseTransformVectorNoScale(Velocity);

		// 변환된 벡터가 가리키는 각도(Yaw)를 구함
		// (정면 W=0, 우측 D=90, 좌측 A=-90, 후진 S=180)
		MovementDirectionAngle = LocalVelocity.Rotation().Yaw;
	}
	else
	{
		MovementDirectionAngle = 0.0f;
	}

	// 제자리 회전 시 발을 움직이기 위한 '초당 회전 속도(Yaw Speed)' 계산
	float CurrentYaw = GetActorRotation().Yaw;
	float YawDelta = CurrentYaw - PreviousActorYaw;

	// 캐릭터가 180도에서 -180도로 순간적으로 넘어갈 때(Wrap) 값이 튀는 현상 방지
	if (YawDelta > 180.0f) YawDelta -= 360.0f;
	if (YawDelta < -180.0f) YawDelta += 360.0f;

	// 초당 회전 속도 산출 (양수면 오른쪽으로 도는 중, 음수면 왼쪽으로 도는 중)
	YawRotationSpeed = YawDelta / DeltaTime;

	// 다음 프레임 계산을 위해 현재 각도 저장
	PreviousActorYaw = CurrentYaw;

	// ==========================================================
	// ★ [새로 추가된 부분] 헤드샷 조준 판별 로직
	bIsAimingAtHead = false; // 매 프레임 일단 false로 초기화

	// 컨트롤러가 유효한지(플레이어가 조종 중인지) 확인
	if (Controller != nullptr)
	{
		FVector CameraLocation;
		FRotator CameraRotation;
		// 현재 활성화된 카메라(조준 카메라든 기본 카메라든)의 정확한 시점을 가져옵니다!
		Controller->GetPlayerViewPoint(CameraLocation, CameraRotation);

		FVector CameraForward = CameraRotation.Vector();

		// 화면 정중앙에서 50m(5000.0f) 앞까지 레이저를 쏩니다.
		FVector TraceEnd = CameraLocation + (CameraForward * 5000.0f);

		FHitResult HitResult;
		FCollisionQueryParams TraceParams;
		TraceParams.AddIgnoredActor(this); // 나 자신은 무시

		if (CurrentWeapon)
		{
			TraceParams.AddIgnoredActor(CurrentWeapon); // 무기도 무시
		}

		if (GetWorld()->LineTraceSingleByChannel(HitResult, CameraLocation, TraceEnd, ECC_Visibility, TraceParams))
		{
			if (HitResult.GetActor() && Cast<ACharacter>(HitResult.GetActor()))
			{
				if (HitResult.BoneName == FName("head"))
				{
					bIsAimingAtHead = true; // 머리 조준 중!
				}
			}
		}
	}
}

// =========================================================================

// Input
void AGun_phiriaCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// 모든 액션에 대해 일관되게 안전 검사(Null Check) 적용
		if (JumpAction)
		{
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
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
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component!"), *GetNameSafe(this));
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

		AddMovementInput(ForwardDirection, MovementVector.Y);
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

// 조준 시작 시 실행되는 로직
void AGun_phiriaCharacter::StartAiming()
{
	bIsAiming = true;

	// 카메라 전환: 3인칭 끄기, 1인칭(가늠좌) 켜기
	if (FollowCamera && ADSCamera)
	{
		FollowCamera->SetActive(false);
		ADSCamera->SetActive(true);
	}
}

// 조준 종료 시 실행되는 로직
void AGun_phiriaCharacter::StopAiming()
{
	bIsAiming = false;

	// 카메라 전환: 1인칭(가늠좌) 끄기, 3인칭 켜기
	if (FollowCamera && ADSCamera)
	{
		ADSCamera->SetActive(false);
		FollowCamera->SetActive(true);
	}
}

void AGun_phiriaCharacter::Fire()
{
	// 무기가 없거나 카메라가 없으면 쏘지 않음
	if (!CurrentWeapon || !FollowCamera || !ADSCamera) return;

	bIsFiring = true;
	GetWorldTimerManager().SetTimer(FireTimerHandle, this, &AGun_phiriaCharacter::StopFiringPose, 1.0f, false);
	LastFireTime = GetWorld()->GetTimeSeconds();

	// =========================================================================

	// 상태에 따른 최종 페널티 계산 (기존과 동일)
	float AimMultiplier = bIsAiming ? 0.6f : 1.2f;
	float MovementMultiplier = (GetVelocity().Size2D() > 10.0f) ? 1.5f : 1.0f;
	float FallMultiplier = GetCharacterMovement()->IsFalling() ? 2.5f : 1.0f;
	float TotalMultiplier = AimMultiplier * MovementMultiplier * FallMultiplier;

	// =========================================================================

	// 물리적 카메라 반동 (Recoil) (기존과 동일)
	if (Controller != nullptr)
	{
		float RecoilPitch = FMath::RandRange(-0.5f, -1.0f) * TotalMultiplier;
		float RecoilYaw = FMath::RandRange(-0.5f, 0.5f) * TotalMultiplier;
		AddControllerPitchInput(RecoilPitch);
		AddControllerYawInput(RecoilYaw);
	}

	// =========================================================================

	// 탄 퍼짐(Spread) 누적
	CurrentSpread = FMath::Clamp(CurrentSpread + (SpreadPerShot * TotalMultiplier), 0.0f, MaxSpread);

	// =========================================================================

	// 목표 지점 계산
	UCameraComponent* ActiveCamera = bIsAiming ? ADSCamera : FollowCamera;
	FVector CameraLocation = ActiveCamera->GetComponentLocation();
	FVector CameraForward = ActiveCamera->GetForwardVector();
	float TraceDistance = 5000.0f;
	FVector CameraEndLocation = CameraLocation + (CameraForward * TraceDistance);

	FHitResult CameraHit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	bool bCameraHit = GetWorld()->LineTraceSingleByChannel(CameraHit, CameraLocation, CameraEndLocation, ECC_Visibility, QueryParams);
	FVector TargetLocation = bCameraHit ? CameraHit.ImpactPoint : CameraEndLocation;

	// =========================================================================
	// ★ 수정된 부분: 고정 거리가 아닌, 실제 타겟까지의 거리를 기준으로 탄 퍼짐 적용

	// 무기에서 무기 고유의 퍼짐 배수를 가져옵니다.
	float SpreadMultiplier = CurrentWeapon->WeaponSpreadMultiplier;
	float FinalSpreadAngle = CurrentSpread * SpreadMultiplier;

	FVector CameraRight = ActiveCamera->GetRightVector();
	FVector CameraUp = ActiveCamera->GetUpVector();

	// [중요] 카메라 위치에서 실제 타겟(벽, 적, 혹은 허공)까지의 거리를 구합니다.
	float ActualDistanceToTarget = FVector::Distance(CameraLocation, TargetLocation);

	// 이 실제 거리를 바탕으로 퍼짐 원판의 반지름을 계산해야, 거리에 맞게 정확히 흩어집니다!
	float SpreadRadius = FMath::Tan(FMath::DegreesToRadians(FinalSpreadAngle)) * ActualDistanceToTarget;

	float RandomAngle = FMath::RandRange(0.0f, PI * 2.0f);
	float RandomRadius = FMath::RandRange(0.0f, SpreadRadius);

	FVector SpreadOffset = (CameraRight * FMath::Cos(RandomAngle) * RandomRadius) + (CameraUp * FMath::Sin(RandomAngle) * RandomRadius);

	// 최종 타겟 지점에 스프레드 오프셋을 더합니다.
	TargetLocation += SpreadOffset;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan,
			FString::Printf(TEXT("Spread Multiplier: %.1f | Spread Angle: %.2f deg"), SpreadMultiplier, FinalSpreadAngle));
	}

	// =========================================================================

	CurrentWeapon->Fire(TargetLocation);
}

// 0.2초 뒤에 사격 자세를 푸는 함수
void AGun_phiriaCharacter::StopFiringPose()
{
	bIsFiring = false;
}

float AGun_phiriaCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead) return 0.0f;

	float ActualDamage = DamageAmount;

	// 포인트 데미지(총알)인지 확인하여 부위 판별 및 이펙트 재생
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FPointDamageEvent* PointDamageEvent = static_cast<const FPointDamageEvent*>(&DamageEvent);
		FName HitBoneName = PointDamageEvent->HitInfo.BoneName;

		// 적이 나를 헤드샷 맞췄을 때 2.5배 데미지!
		if (HitBoneName == FName("head"))
		{
			ActualDamage *= 2.5f;
		}

		// 플레이어 피격 이펙트 재생 (적과 동일하게 피 튀김)
		if (PlayerHitEffect)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), PlayerHitEffect, PointDamageEvent->HitInfo.ImpactPoint, PointDamageEvent->HitInfo.ImpactNormal.Rotation());
		}
	}

	ActualDamage = Super::TakeDamage(ActualDamage, DamageEvent, EventInstigator, DamageCauser);

	// 체력 깎기
	CurrentHealth -= ActualDamage;
	CurrentHealth = FMath::Clamp(CurrentHealth, 0.0f, MaxHealth);

	// 체력이 0이 되면 사망 처리
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

	// 플레이어 조작 비활성화 및 래그돌(물리 쓰러짐) 켜기
	DisableInput(Cast<APlayerController>(GetController()));
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	GetMesh()->SetSimulatePhysics(true);

	// 화면에 크게 사망 메시지 띄우기
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("YOU DIED"));
	}
}