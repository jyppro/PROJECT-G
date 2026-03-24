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
	ADSCamera->SetupAttachment(GetMesh());

	// 카메라는 마우스가 아닌 '총의 애니메이션 흔들림'을 그대로 따라가야 하므로 false로 설정
	ADSCamera->bUsePawnControlRotation = false;

	// 평소(비조준 상태)에는 꺼져 있어야 하므로 기본 활성화 상태를 false로 둠
	ADSCamera->SetAutoActivate(false);

	ADSCamera->PrimaryComponentTick.TickGroup = TG_PostPhysics;

	// 캡슐 컴포넌트는 총알(Visibility)을 무시(Ignore)
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);

	// 스켈레탈 메시는 충돌 연산을 켜주고, 총알(Visibility)을 Block
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	// ★ [새로 추가] 캐릭터 움직임 컴포넌트에서 숙이기 기능을 사용하겠다고 활성화합니다!
	// (이게 꺼져있으면 Crouch() 함수를 불러도 작동하지 않습니다.)
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

		// (선택) 숙여서 걸을 때 속도를 조절합니다. (기존 500의 절반인 250 정도로 설정)
		GetCharacterMovement()->MaxWalkSpeedCrouched = 250.0f;
	}

	bIsProne = false;
	DefaultCapsuleHalfHeight = 96.0f;
	DefaultMeshRelativeLocationZ = -97.0f; // 프로젝트 메쉬 위치에 맞게 조절하세요 (보통 -90 ~ -97)
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
		// 땅에 있지만 움직일 때 (이동 중에도 자세에 따라 퍼짐이 다름)
		if (bIsProne)
		{
			// 기어갈 때
			TargetMinSpread = bIsAiming ? 0.5f : 1.5f;
		}
		else if (bIsCrouched)
		{
			// 앉아서 걸을 때
			TargetMinSpread = bIsAiming ? 1.0f : 2.0f;
		}
		else
		{
			// 서서 걷거나 뛸 때
			TargetMinSpread = bIsAiming ? 1.5f : 3.0f;
		}
	}
	else
	{
		// 제자리에 멈춰 있을 때 (★ 배틀그라운드처럼 자세별 세분화 적용)
		if (bIsProne)
		{
			// 엎드려쏴 (가장 안정적, 비조준 시에도 퍼짐이 매우 적음)
			TargetMinSpread = bIsAiming ? 0.0f : 0.2f;
		}
		else if (bIsCrouched)
		{
			// 앉아쏴 (서있을 때보다 안정적)
			// 조준 시 미세한 흔들림을 원하면 0.0f 대신 0.05f 등으로 설정 가능
			TargetMinSpread = bIsAiming ? 0.0f : 0.5f;
		}
		else
		{
			// 서서쏴 (가장 불안정적, 기본값)
			TargetMinSpread = bIsAiming ? 0.0f : 1.0f;
		}
	}

	float CurrentTime = GetWorld()->GetTimeSeconds(); // 현재 게임 플레이 시간 가져오기

	if (CurrentTime - LastFireTime > SpreadRecoveryDelay)
	{
		// 사격한 지 SpreadRecoveryDelay(예: 0.2초)가 지났을 때만 서서히 회복
		CurrentSpread = FMath::FInterpTo(CurrentSpread, TargetMinSpread, DeltaTime, SpreadRecoveryRate);
	}

	// --- 절차적 조준(Procedural ADS) 뼈대 타겟 좌표 계산 ---

	// 1. 조준 상태에 따라 0.0(비조준) ~ 1.0(조준)으로 부드럽게 변하는 보간값 생성
	float TargetAlpha = bIsAiming ? 1.0f : 0.0f;
	// 15.0f는 조준하는 속도야. 필요에 따라 더 빠르거나 느리게 조절할 수 있어.
	ADSAlpha = FMath::FInterpTo(ADSAlpha, TargetAlpha, DeltaTime, 15.0f);

	// 2. 무기가 유효하고, 알파값이 0보다 커서 조준 연산이 필요할 때만 실행
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
		FVector SightToHandOffset = HandWorldLoc - SightWorldLoc;

		FVector TargetHandWorldLoc = TargetSightWorldLoc + SightToHandOffset;

		FTransform MeshTransform = GetMesh()->GetComponentTransform();
		FVector TargetHandCS = MeshTransform.InverseTransformPosition(TargetHandWorldLoc);

		DynamicAimOffset = FMath::Lerp(FVector::ZeroVector, TargetHandCS, ADSAlpha);
	}
	else if (ADSAlpha <= 0.01f)
	{
		// 비조준 상태일 때는 오프셋을 0으로 초기화
		DynamicAimOffset = FVector::ZeroVector;
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

	// 헤드샷 조준 판별 로직
	bIsAimingAtHead = false; // 매 프레임 일단 false로 초기화

	// 컨트롤러가 유효한지(플레이어가 조종 중인지) 확인
	if (Controller != nullptr)
	{
		FVector CameraLocation;
		FRotator CameraRotation;
		// 현재 활성화된 카메라(조준 카메라든 기본 카메라든)의 정확한 시점을 가져옴
		Controller->GetPlayerViewPoint(CameraLocation, CameraRotation);

		FVector CameraForward = CameraRotation.Vector();

		// 화면 정중앙에서 50m(5000.0f) 앞까지 레이저 쏘기
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
					bIsAimingAtHead = true; // 머리 조준 중
				}
			}
		}
	}

	CurrentLean = FMath::FInterpTo(CurrentLean, TargetLean, DeltaTime, 10.0f);

	if (FollowCamera && ADSCamera && GetMesh())
	{
		FVector PureAimForward = GetBaseAimRotation().Vector();

		// 이 순수 조준 방향을 캐릭터 몸통(Component Space) 축으로 변환합니다.
		LeanAxisCS = GetMesh()->GetComponentTransform().InverseTransformVectorNoScale(PureAimForward);
	}

	// ★ [새로 추가] 비조준 상태(FollowCamera)의 좌우 시야 확보 로직
	if (CameraBoom)
	{
		// 1. 기본 위치 (평소 카메라 위치)
		FVector TargetOffset = FVector(0.0f, 0.0f, 120.0f);

		// 2. 기울기(CurrentLean: -1.0 ~ 1.0)에 따라 좌우(Y축)로 얼마나 밀어줄지 계산
		// 100.0f는 카메라가 옆으로 나가는 거리입니다. (테스트 후 조절 가능)
		float LeanOffsetAmount = CurrentLean * 100.0f;

		// 3. 목표 오프셋 설정 (Y값에 기울기 반영)
		TargetOffset.Y = LeanOffsetAmount;

		// 4. 부드럽게 카메라 위치 이동 (뚝 끊기지 않게 Interp 사용)
		CameraBoom->SocketOffset = FMath::VInterpTo(CameraBoom->SocketOffset, TargetOffset, DeltaTime, 10.0f);
	}

	// ★ [총기 벽 뚫림 완벽 방지] 총구가 벽에 닿으면 캐릭터를 부드럽게 뒤로 밀어냅니다!
	// ==========================================================
	if (CurrentWeapon && CurrentWeapon->GetWeaponMesh())
	{
		// 실제 총구 위치
		FVector MuzzleLoc = CurrentWeapon->GetWeaponMesh()->GetSocketLocation(FName("MuzzleSocket"));

		// 1. 캐릭터 중심에서 총구와 같은 높이의 시작점을 만듭니다. 
		// (높이를 맞추는 이유: 창문 너머로 사격할 때 창틀에 밀려나지 않기 위해)
		FVector Start = GetActorLocation();
		Start.Z = MuzzleLoc.Z;

		// 2. 중심에서 총구를 향하는 방향을 구합니다.
		FVector Direction = (MuzzleLoc - Start).GetSafeNormal();

		// 3. 총구 위치보다 살짝 앞(여유공간 15cm)을 끝점으로 잡습니다.
		FVector End = MuzzleLoc + (Direction * 15.0f);

		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);
		Params.AddIgnoredActor(CurrentWeapon);

		// 4. 캐릭터 가슴에서 총구 방향으로 레이저를 쏴서 벽이 있는지 스캔합니다.
		if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
		{
			// 5. 바닥이나 천장이 아닌 '벽'일 때만 작동하도록 합니다. (평면의 수직 각도 체크)
			if (FMath::Abs(Hit.ImpactNormal.Z) < 0.3f)
			{
				// 총구가 벽을 파고든 깊이를 계산합니다.
				float PenetrationDepth = FVector::Distance(Hit.ImpactPoint, End);

				// 벽의 수직 방향(ImpactNormal)으로 캐릭터를 밀어냅니다!
				FVector PushDirection = Hit.ImpactNormal;
				PushDirection.Z = 0.0f; // 위아래로는 밀리지 않도록 고정
				PushDirection.Normalize();

				// bSweep을 true로 켜서 뒤로 밀려날 때 다른 벽을 뚫지 않도록 안전하게 이동시킵니다.
				AddActorWorldOffset(PushDirection * PenetrationDepth, true);
			}
		}
	}
}

// Input
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
		if (CrouchAction)
		{
			EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &AGun_phiriaCharacter::ToggleCrouch);
		}
		if (LeanAction)
		{
			// 키를 누르고 있는 동안(Triggered)
			EnhancedInputComponent->BindAction(LeanAction, ETriggerEvent::Triggered, this, &AGun_phiriaCharacter::InputLean);
			// 키를 완전히 뗐을 때(Completed)
			EnhancedInputComponent->BindAction(LeanAction, ETriggerEvent::Completed, this, &AGun_phiriaCharacter::InputLeanEnd);
		}
		if (ProneAction)
		{
			EnhancedInputComponent->BindAction(ProneAction, ETriggerEvent::Started, this, &AGun_phiriaCharacter::ToggleProne);
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

		float ForwardInput = MovementVector.Y; // W키(+1) 또는 S키(-1) 입력값

		// ==========================================================
		// ★ [총구 벽 뚫림 방지 로직] 총구 쪽에 벽 충돌을 감지하여 전진을 차단

		// 앞으로 가려고 할 때(ForwardInput > 0)만 벽을 체크합니다. (뒤로 가는 건 허용)
		if (CurrentWeapon && CurrentWeapon->GetWeaponMesh() && ForwardInput > 0.0f)
		{
			// 1. 실제 총구 위치를 가져옵니다.
			FVector MuzzleLocation = CurrentWeapon->GetWeaponMesh()->GetSocketLocation(FName("MuzzleSocket"));

			// 2. 센서 시작점(총구 살짝 뒤)과 끝점(총구 살짝 앞)을 설정합니다.
			// 총구 안쪽에서부터 트레이스를 쏴야 이미 살짝 파묻혔을 때도 정확히 감지해냅니다.
			FVector TraceStart = MuzzleLocation - (ForwardDirection * 20.0f);
			FVector TraceEnd = MuzzleLocation + (ForwardDirection * 15.0f); // 총구 앞 15cm까지 감지

			FHitResult Hit;
			FCollisionQueryParams Params;
			Params.AddIgnoredActor(this);
			Params.AddIgnoredActor(CurrentWeapon); // 나와 내 총은 무시

			// 3. 얇은 선(Line) 대신 총 두께만한 구슬(Sphere)을 쏴서 더 정확하게 벽을 감지합니다.
			FCollisionShape Sphere = FCollisionShape::MakeSphere(10.0f); // 반지름 10cm 구슬

			// 4. 총구 앞에 벽이 있는지 스윕(Sweep) 검사!
			if (GetWorld()->SweepSingleByChannel(Hit, TraceStart, TraceEnd, FQuat::Identity, ECC_Visibility, Sphere, Params))
			{
				// 벽이 감지되었다면? 전진 입력(W)을 강제로 0으로 만들어버립니다!
				ForwardInput = 0.0f;

				// (디버그 확인용 - 코드가 잘 작동하는지 붉은 구슬을 그려줍니다. 확인 후 지우셔도 됩니다.)
				DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 10.0f, 12, FColor::Red, false, 0.1f);
			}
		}

		// 수정된 ForwardInput을 사용해 전진/후진 적용 (벽에 닿으면 0이 되어 딱 멈춥니다!)
		AddMovementInput(ForwardDirection, ForwardInput);

		// 좌우 이동(A, D)은 터치하지 않았으므로, 벽에 막혀도 게걸음으로 옆으로는 스르륵 이동할 수 있습니다.
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

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && CurrentWeapon)
	{
		// 재생할 몽타주 목록을 담을 임시 배열
		TArray<UAnimMontage*> MontagesToPlay;

		// 1. 현재 자세에 따라 무기에서 어떤 몽타주 배열을 가져올지 결정
		if (bIsProne)
		{
			MontagesToPlay = CurrentWeapon->ProneFireMontages;
		}
		else // 서 있거나 앉은 상태
		{
			MontagesToPlay = CurrentWeapon->StandCrouchFireMontages;
		}

		// 2. 해당 무기에 등록된 애니메이션이 1개라도 있는지 확인!
		// (권총처럼 비워두면 0이므로 이 조건문을 무시하고 넘어감 -> 에러 방지 및 확장성 확보)
		if (MontagesToPlay.Num() > 0)
		{
			// 등록된 애니메이션 중 하나를 랜덤으로 재생
			int32 RandomIndex = FMath::RandRange(0, MontagesToPlay.Num() - 1);
			AnimInstance->Montage_Play(MontagesToPlay[RandomIndex]);
		}
	}

	// 상태에 따른 최종 페널티 계산
	float AimMultiplier = bIsAiming ? 0.6f : 1.2f;
	float MovementMultiplier = (GetVelocity().Size2D() > 10.0f) ? 1.5f : 1.0f;
	float FallMultiplier = GetCharacterMovement()->IsFalling() ? 2.5f : 1.0f;

	float StanceMultiplier = 1.0f; // 기본: 서 있는 상태 (반동 100%)

	if (bIsProne)
	{
		StanceMultiplier = 0.4f; // 엎드려 쏠 때: 반동 및 퍼짐 60% 감소 (매우 안정적)
	}
	else if (bIsCrouched)
	{
		StanceMultiplier = 0.75f; // 앉아 쏠 때: 반동 및 퍼짐 25% 감소 (조금 안정적)
	}

	// 모든 배율을 곱해서 최종 배율(TotalMultiplier) 산출
	float TotalMultiplier = AimMultiplier * MovementMultiplier * FallMultiplier * StanceMultiplier;

	// 물리적 카메라 반동 (Recoil)
	if (Controller != nullptr)
	{
		// Pitch(상하) 반동: 위로 튀어야 하므로 음수값, TotalMultiplier로 자세별 감소 적용
		float RecoilPitch = FMath::RandRange(-0.5f, -1.0f) * TotalMultiplier;
		// Yaw(좌우) 반동: 좌우 랜덤, TotalMultiplier로 자세별 감소 적용
		float RecoilYaw = FMath::RandRange(-0.5f, 0.5f) * TotalMultiplier;

		AddControllerPitchInput(RecoilPitch);
		AddControllerYawInput(RecoilYaw);
	}

	// 탄 퍼짐(Spread) 누적: 총을 쏠 때 순간적으로 벌어지는 수치도 자세의 영향을 받게 됨
	CurrentSpread = FMath::Clamp(CurrentSpread + (SpreadPerShot * TotalMultiplier), 0.0f, MaxSpread);

	// ==========================================================
	// 실제 총구에서 조준점을 향해 쏘는 로직

	// 1. [시선 계산] 카메라가 현재 바라보고 있는 월드상의 목표 지점(EyeTarget)을 먼저 찾습니다.
	UCameraComponent* ActiveCamera = bIsAiming ? ADSCamera : FollowCamera;
	FVector CameraLocation = ActiveCamera->GetComponentLocation();
	FVector CameraForward = ActiveCamera->GetForwardVector();
	float TraceDistance = 10000.0f;
	FVector CameraEndLocation = CameraLocation + (CameraForward * TraceDistance);

	FHitResult CameraHit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	if (CurrentWeapon) QueryParams.AddIgnoredActor(CurrentWeapon);

	FVector EyeTargetLocation;
	if (GetWorld()->LineTraceSingleByChannel(CameraHit, CameraLocation, CameraEndLocation, ECC_Visibility, QueryParams))
	{
		EyeTargetLocation = CameraHit.ImpactPoint; // 무언가 맞았다면 그 지점이 조준점
	}
	else
	{
		EyeTargetLocation = CameraEndLocation; // 아무것도 없다면 허공 끝이 조준점
	}

	// 2. [총구 위치] 실제 무기 메시에 설정된 소켓 위치를 가져옵니다.
	FVector MuzzleLocation = CurrentWeapon->GetWeaponMesh()->GetSocketLocation(FName("MuzzleSocket"));

	// 3. [탄도 정렬] 총구에서 시선 목표 지점(EyeTarget)을 향하는 방향 벡터를 구합니다.
	FVector RealFireDirection = (EyeTargetLocation - MuzzleLocation).GetSafeNormal();

	// 4. [실제 탄도 체크] 총구에서 목표 지점 방향으로 진짜 총알 궤적 레이저를 쏩니다.
	FVector FinalTraceEnd = MuzzleLocation + (RealFireDirection * 5000.0f);
	FHitResult FinalHit;

	FVector FinalTargetLocation; // 최종적으로 탄착군(Spread)이 형성될 중심점

	// 실제 총구 앞에 벽이 있는지 체크
	if (GetWorld()->LineTraceSingleByChannel(FinalHit, MuzzleLocation, FinalTraceEnd, ECC_Visibility, QueryParams))
	{
		FinalTargetLocation = FinalHit.ImpactPoint;
	}
	else
	{
		FinalTargetLocation = FinalTraceEnd;
	}

	// --- 탄 퍼짐(Spread) 계산 (총구 기준 벡터로 계산) ---
	float SpreadMultiplier = CurrentWeapon->WeaponSpreadMultiplier;
	float FinalSpreadAngle = CurrentSpread * SpreadMultiplier;

	// 총구 방향을 기준으로 Right와 Up 벡터 생성
	FVector Forward, Right, Up;
	RealFireDirection.FindBestAxisVectors(Right, Up);

	float ActualDistanceToTarget = FVector::Distance(MuzzleLocation, FinalTargetLocation);
	float SpreadRadius = FMath::Tan(FMath::DegreesToRadians(FinalSpreadAngle)) * ActualDistanceToTarget;

	float RandomAngle = FMath::RandRange(0.0f, PI * 2.0f);
	float RandomRadius = FMath::RandRange(0.0f, SpreadRadius);

	FVector SpreadOffset = (Right * FMath::Cos(RandomAngle) * RandomRadius) + (Up * FMath::Sin(RandomAngle) * RandomRadius);

	// 최종 타겟 지점 확정
	FVector TargetLocation = FinalTargetLocation + SpreadOffset;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan,
			FString::Printf(TEXT("Spread Multiplier: %.1f | Spread Angle: %.2f deg"), SpreadMultiplier, FinalSpreadAngle));
	}

	// 진짜 총구 위치에서 조준된 목표 지점으로 발사 명령
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

		// 적이 나를 헤드샷 맞췄을 때 2.5배 데미지
		if (HitBoneName == FName("head"))
		{
			ActualDamage *= 2.5f;
		}
	}

	ActualDamage = Super::TakeDamage(ActualDamage, DamageEvent, EventInstigator, DamageCauser);

	if (HitMontages.Num() > 0)
	{
		// 0번부터 (배열 개수 - 1)번 사이의 숫자를 랜덤으로 하나 뽑습니다.
		int32 RandomIndex = FMath::RandRange(0, HitMontages.Num() - 1);

		// 당첨된 번호의 몽타주를 재생합니다!
		// 플레이어 전용 애님 인스턴스를 가져와서 몽타주를 재생하도록 명령합니다.
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			AnimInstance->Montage_Play(HitMontages[RandomIndex]);
		}
	}

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

void AGun_phiriaCharacter::ToggleCrouch()
{
	if (bIsProne) return;

	// 언리얼 ACharacter에 기본 내장된 변수 bIsCrouched를 확인합니다.
	if (bIsCrouched)
	{
		// 이미 숙이고 있다면 일어섭니다.
		UnCrouch();
	}
	else
	{
		// 서 있다면 숙입니다.
		Crouch();
	}
}

void AGun_phiriaCharacter::InputLean(const FInputActionValue& Value)
{
	// E키면 1.0, Q키면 -1.0이 들어옵니다.
	TargetLean = Value.Get<float>();
}

void AGun_phiriaCharacter::InputLeanEnd(const FInputActionValue& Value)
{
	// 키를 떼면 다시 똑바로 섭니다.
	TargetLean = 0.0f;
}

void AGun_phiriaCharacter::ToggleProne()
{
	// 공중에 떠있을 때는 엎드릴 수 없도록 방지
	if (GetCharacterMovement()->IsFalling()) return;

	if (bIsProne)
	{
		// [엎드림 -> 일어서기]
		bIsProne = false;

		// 1. 캡슐 크기를 원래대로 (96.0f)
		GetCapsuleComponent()->SetCapsuleHalfHeight(DefaultCapsuleHalfHeight);

		// 2. 메쉬 위치를 원래대로 복구
		FVector MeshLoc = GetMesh()->GetRelativeLocation();
		MeshLoc.Z = DefaultMeshRelativeLocationZ;
		GetMesh()->SetRelativeLocation(MeshLoc);

		// 3. 이동 속도를 원래대로 (500.0f)
		GetCharacterMovement()->MaxWalkSpeed = 500.0f;
	}
	else
	{
		// [서있음/앉아있음 -> 엎드리기]
		// 만약 앉아있는 상태라면 먼저 앉기 상태를 해제합니다.
		if (bIsCrouched)
		{
			UnCrouch();
		}

		bIsProne = true;

		// 1. 캡슐 높이를 대폭 줄입니다 (예: 34.0f)
		float ProneHalfHeight = 34.0f;
		GetCapsuleComponent()->SetCapsuleHalfHeight(ProneHalfHeight);

		// 2. 캡슐이 줄어든 만큼 메쉬가 공중에 뜨지 않게 위로 올려줍니다.
		// 높이 차이 = 96.0 - 34.0 = 62.0. 기존 Z위치(-97) + 62 = -35.0
		float HeightDifference = DefaultCapsuleHalfHeight - ProneHalfHeight;
		FVector MeshLoc = GetMesh()->GetRelativeLocation();
		MeshLoc.Z = DefaultMeshRelativeLocationZ + HeightDifference;
		GetMesh()->SetRelativeLocation(MeshLoc);

		// 3. 이동 속도를 기어가는 속도로 변경
		GetCharacterMovement()->MaxWalkSpeed = MaxWalkSpeedProne;
	}
}