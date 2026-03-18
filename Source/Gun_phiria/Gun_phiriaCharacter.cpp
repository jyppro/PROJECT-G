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


DEFINE_LOG_CATEGORY(LogTemplateCharacter);

AGun_phiriaCharacter::AGun_phiriaCharacter()
{
	// Tick 함수 작동 - 카메라 스무스 줌
	PrimaryActorTick.bCanEverTick = true;

	// 변수 초기화
	PreviousActorYaw = 0.0f;
	MovementDirectionAngle = 0.0f;
	YawRotationSpeed = 0.0f;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// 마우스(컨트롤러)가 회전할 때 캐릭터도 같이 좌우로 회전
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;  // false에서 true로 변경
	bUseControllerRotationRoll = false;

	// 캐릭터가 입력 방향을 바라보며 뛰어가는 기능 끄기 (게걸음, 뒷걸음질 활성화)
	GetCharacterMovement()->bOrientRotationToMovement = false; // true에서 false로 변경
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

	// 무기 컴포넌트 생성 및 손에 부착 (소켓 이름은 블루프린트와 동일하게 "WeaponSocket")
	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(GetMesh(), FName("WeaponSocket"));
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 총은 충돌 처리 X

	// --- 조준 전용 카메라 생성 및 가늠좌 소켓에 부착 ---
	ADSCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ADSCamera"));
	// WeaponMesh의 "SightSocket"에 정확히 부착합니다.
	ADSCamera->SetupAttachment(WeaponMesh, FName("SightSocket"));

	// 카메라는 마우스가 아닌 '총의 애니메이션 흔들림'을 그대로 따라가야 하므로 false로 설정
	ADSCamera->bUsePawnControlRotation = false;

	// 평소(비조준 상태)에는 꺼져 있어야 하므로 기본 활성화 상태를 false로 둠
	ADSCamera->SetAutoActivate(false);
}

void AGun_phiriaCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (CameraBoom)
	{
		DefaultSocketOffset = CameraBoom->SocketOffset;
	}
}

void AGun_phiriaCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 1. 카메라 이동 로직 (기존 코드 유지)
	if (FollowCamera && ADSCamera)
	{
		float TargetFOV = bIsAiming ? AimFOV : DefaultFOV;
		if (bIsAiming)
		{
			float NewFOV = FMath::FInterpTo(ADSCamera->FieldOfView, TargetFOV, DeltaTime, ZoomInterpSpeed);
			ADSCamera->SetFieldOfView(NewFOV);
		}
		else
		{
			float NewFOV = FMath::FInterpTo(FollowCamera->FieldOfView, TargetFOV, DeltaTime, ZoomInterpSpeed);
			FollowCamera->SetFieldOfView(NewFOV);
		}
	}

	// =========================================================================
	// [수정된 로직] 플레이어 상태(이동, 점프)에 따른 탄 퍼짐 기본값 설정
	// =========================================================================
	float CurrentSpeed = GetVelocity().Size2D();
	bool bIsFalling = GetCharacterMovement()->IsFalling(); // 공중에 떠 있는지 확인

	float TargetMinSpread = 0.0f; // 기본 수치

	if (bIsFalling)
	{
		// 1순위: 공중에 떠 있을 때 (가장 큰 페널티)
		TargetMinSpread = bIsAiming ? 4.0f : 8.0f;
	}
	else if (CurrentSpeed > 10.0f)
	{
		// 2순위: 땅에 있지만 걷거나 뛸 때
		TargetMinSpread = bIsAiming ? 1.5f : 4.0f;
	}
	else
	{
		// 3순위: 땅에서 가만히 서 있을 때
		TargetMinSpread = bIsAiming ? 0.0f : 1.0f; // 서 있을 때도 비조준 시 약간의 퍼짐 유지
	}

	// 탄 퍼짐 수치 서서히 회복
	// CurrentSpread = FMath::FInterpTo(CurrentSpread, TargetMinSpread, DeltaTime, SpreadRecoveryRate);

	float CurrentTime = GetWorld()->GetTimeSeconds(); // 현재 게임 플레이 시간 가져오기

	if (CurrentTime - LastFireTime > SpreadRecoveryDelay)
	{
		// 사격한 지 SpreadRecoveryDelay(예: 0.2초)가 지났을 때만 서서히 회복
		CurrentSpread = FMath::FInterpTo(CurrentSpread, TargetMinSpread, DeltaTime, SpreadRecoveryRate);
	}
	// =========================================================================


	// --- 절차적 조준(Procedural ADS) 뼈대 타겟 좌표 계산 (기존 코드 유지) ---
	if (bIsAiming && FollowCamera && WeaponMesh)
	{
		FVector CameraLocation = FollowCamera->GetComponentLocation();
		FVector CameraForward = FollowCamera->GetForwardVector();

		FVector CamToChar = GetActorLocation() - CameraLocation;
		float ActualDistance = FVector::DotProduct(CamToChar, CameraForward);
		float TotalDistance = ActualDistance + AimDistance;

		FVector TargetSightWorldLoc = CameraLocation + (CameraForward * TotalDistance);

		FVector HandWorldLoc = WeaponMesh->GetComponentLocation();
		FVector SightWorldLoc = WeaponMesh->GetSocketLocation(FName("SightSocket"));
		FVector SightToHandOffset = HandWorldLoc - SightWorldLoc;

		FVector TargetHandWorldLoc = TargetSightWorldLoc + SightToHandOffset;

		FTransform MeshTransform = GetMesh()->GetComponentTransform();
		FVector TargetHandCS = MeshTransform.InverseTransformPosition(TargetHandWorldLoc);

		DynamicAimOffset = FMath::VInterpTo(DynamicAimOffset, TargetHandCS, DeltaTime, 20.0f);
	}

	// =========================================================================
	// [새로운 로직 1] 대각선 이동 애니메이션을 위한 '이동 방향 각도(Direction)' 계산
	// =========================================================================
	FVector Velocity = GetVelocity();
	if (Velocity.Size2D() > 1.0f) // 캐릭터가 조금이라도 움직이고 있다면
	{
		// 월드 기준의 속도 벡터를 캐릭터 기준(Local) 벡터로 변환합니다.
		FVector LocalVelocity = GetActorTransform().InverseTransformVectorNoScale(Velocity);

		// 변환된 벡터가 가리키는 각도(Yaw)를 구합니다. 
		// (정면 W=0, 우측 D=90, 좌측 A=-90, 후진 S=180)
		MovementDirectionAngle = LocalVelocity.Rotation().Yaw;
	}
	else
	{
		MovementDirectionAngle = 0.0f;
	}

	// =========================================================================
	// [새로운 로직 2] 제자리 회전 시 발을 움직이기 위한 '초당 회전 속도(Yaw Speed)' 계산
	// =========================================================================
	float CurrentYaw = GetActorRotation().Yaw;
	float YawDelta = CurrentYaw - PreviousActorYaw;

	// 캐릭터가 180도에서 -180도로 순간적으로 넘어갈 때(Wrap) 값이 튀는 현상 방지
	if (YawDelta > 180.0f) YawDelta -= 360.0f;
	if (YawDelta < -180.0f) YawDelta += 360.0f;

	// 초당 회전 속도 산출 (양수면 오른쪽으로 도는 중, 음수면 왼쪽으로 도는 중)
	YawRotationSpeed = YawDelta / DeltaTime;

	// 다음 프레임 계산을 위해 현재 각도 저장
	PreviousActorYaw = CurrentYaw;
	// =========================================================================
}

//////////////////////////////////////////////////////////////////////////
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
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {

		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AGun_phiriaCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AGun_phiriaCharacter::Look);

		// AimAction (우클릭) 바인딩
		if (AimAction)
		{
			EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &AGun_phiriaCharacter::StartAiming);
			EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &AGun_phiriaCharacter::StopAiming);
		}

		// FireAction (좌클릭) 바인딩
		if (FireAction)
		{
			// 좌클릭을 누르는 순간(Started) Fire 함수 딱 한 번 실행! (단발 사격)
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
	if (!FollowCamera || !ADSCamera || !WeaponMesh) return;

	bIsFiring = true;
	GetWorldTimerManager().SetTimer(FireTimerHandle, this, &AGun_phiriaCharacter::StopFiringPose, 1.0f, false);

	LastFireTime = GetWorld()->GetTimeSeconds();


	// =========================================================================
	// 1. 상태에 따른 최종 페널티 계산 (기존 동일)
	// =========================================================================
	float AimMultiplier = bIsAiming ? 0.6f : 1.2f;
	float MovementMultiplier = (GetVelocity().Size2D() > 10.0f) ? 1.5f : 1.0f;
	float FallMultiplier = GetCharacterMovement()->IsFalling() ? 2.5f : 1.0f;

	float TotalMultiplier = AimMultiplier * MovementMultiplier * FallMultiplier;

	// =========================================================================
	// 2. 물리적 카메라 반동 (Recoil)
	// =========================================================================
	if (Controller != nullptr)
	{
		float RecoilPitch = FMath::RandRange(-0.5f, -1.0f) * TotalMultiplier;
		float RecoilYaw = FMath::RandRange(-0.5f, 0.5f) * TotalMultiplier;

		AddControllerPitchInput(RecoilPitch);
		AddControllerYawInput(RecoilYaw);
	}

	// =========================================================================
	// 3. 탄 퍼짐(Spread) 누적
	// =========================================================================
	CurrentSpread = FMath::Clamp(CurrentSpread + (SpreadPerShot * TotalMultiplier), 0.0f, MaxSpread);

	UCameraComponent* ActiveCamera = bIsAiming ? ADSCamera : FollowCamera;

	// STEP 1: 화면 정중앙(카메라)이 가리키는 최종 타겟 지점 찾기
	FVector CameraLocation = ActiveCamera->GetComponentLocation();
	FVector CameraForward = ActiveCamera->GetForwardVector();
	float TraceDistance = 5000.0f; // 50미터
	FVector CameraEndLocation = CameraLocation + (CameraForward * TraceDistance);

	FHitResult CameraHit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this); // 내 캐릭터 무시

	bool bCameraHit = GetWorld()->LineTraceSingleByChannel(
		CameraHit, CameraLocation, CameraEndLocation, ECC_Visibility, QueryParams
	);

	FVector TargetLocation = bCameraHit ? CameraHit.ImpactPoint : CameraEndLocation;

	// STEP 2: 실제 총구(Muzzle)에서 타겟 지점을 향해 총알 발사
	FVector MuzzleLocation = WeaponMesh->GetSocketLocation(FName("MuzzleSocket"));
	FVector BaseDirection = (TargetLocation - MuzzleLocation).GetSafeNormal();

	// =========================================================================
	// [수정 및 확인 포인트] 실제 총알 궤적에 탄 퍼짐 적용하기
	// =========================================================================
	// 기존 0.5f 대신 눈에 확실히 띄게 수치를 올려봅니다. (테스트용)
	// 만약 잘 흩어진다면 이 SpreadMultiplier 값을 조정하여 알맞은 감도를 찾으세요!
	float SpreadMultiplier = 3.0f;
	float FinalSpreadAngle = CurrentSpread * SpreadMultiplier;

	// 각도를 라디안으로 변환
	float HalfConeAngle = FMath::DegreesToRadians(FinalSpreadAngle);

	// BaseDirection을 중심으로 HalfConeAngle만큼 무작위 원뿔 형태로 흩어집니다.
	FVector SpreadDirection = FMath::VRandCone(BaseDirection, HalfConeAngle);

	// 퍼짐이 적용된 최종 목적지 구하기
	FVector BulletEndLocation = MuzzleLocation + (SpreadDirection * TraceDistance);

	// 디버그용: 현재 내 총알이 몇 도의 각도로 흩어지고 있는지 화면 좌측 상단에 출력
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan,
			FString::Printf(TEXT("적용된 퍼짐 각도: %.2f도 (현재 Spread값: %.2f)"), FinalSpreadAngle, CurrentSpread));
	}
	// =========================================================================

	FHitResult BulletHit;

	/// 실제 총알 발사 (Line Trace)
	bool bBulletHit = GetWorld()->LineTraceSingleByChannel(
		BulletHit, MuzzleLocation, BulletEndLocation, ECC_Visibility, QueryParams
	);

	// =========================================================================
	// [새로운 로직] 시각적 이펙트(나이아가라) 스폰하기
	// =========================================================================

	// 1. 총구 화염 (Muzzle Flash) 터뜨리기
	if (MuzzleFlashEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			MuzzleFlashEffect, WeaponMesh, FName("MuzzleSocket"),
			FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true
		);
	}

	// 2. 총알 궤적 (Tracer) 그리기
	if (BulletTracerEffect)
	{
		// 총구에서 이펙트를 생성합니다.
		UNiagaraComponent* TracerComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(), BulletTracerEffect, MuzzleLocation
		);

		if (TracerComponent)
		{
			// 이펙트의 끝점(도착점)을 설정해줍니다. (나이아가라 변수 이름이 "Target"이라고 가정)
			FVector TracerEnd = bBulletHit ? BulletHit.ImpactPoint : BulletEndLocation;
			TracerComponent->SetVectorParameter(FName("Target"), TracerEnd);
		}
	}

	// 3. 충돌 결과 처리
	if (bBulletHit)
	{
		// 디버그용 (나중엔 지우셔도 됩니다)
		if (GEngine && BulletHit.GetActor())
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, FString::Printf(TEXT("명중: %s"), *BulletHit.GetActor()->GetName()));
		}

		// 벽이나 적에 맞았을 때 피격 이펙트 (Impact) 터뜨리기
		if (ImpactEffect)
		{
			// 맞은 표면의 방향(Normal)을 바라보게 이펙트를 회전시켜 생성합니다.
			FRotator ImpactRotation = BulletHit.ImpactNormal.Rotation();
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(), ImpactEffect, BulletHit.ImpactPoint, ImpactRotation
			);
		}
	}
}

// 0.2초 뒤에 사격 자세를 푸는 함수
void AGun_phiriaCharacter::StopFiringPose()
{
	bIsFiring = false;
}