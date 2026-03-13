#include "Gun_phiriaCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "DrawDebugHelpers.h"  // 총알 궤적(레이저)을 눈으로 보기 위해 필수!

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AGun_phiriaCharacter

AGun_phiriaCharacter::AGun_phiriaCharacter()
{
	// Tick 함수가 작동하도록 설정 (카메라 스무스 줌을 위해 필수!)
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// 무기 컴포넌트 생성 및 손에 부착 (소켓 이름은 블루프린트와 동일하게 "WeaponSocket")
	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(GetMesh(), FName("WeaponSocket"));
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 총은 충돌 처리가 필요 없으므로 끔

	// --- [여기에 새 코드 추가] 조준 전용 카메라 생성 및 가늠좌 소켓에 부착 ---
	ADSCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ADSCamera"));
	// WeaponMesh의 "SightSocket"에 정확히 부착합니다.
	ADSCamera->SetupAttachment(WeaponMesh, FName("SightSocket"));

	// 카메라는 마우스가 아닌 '총의 애니메이션 흔들림'을 그대로 따라가야 하므로 false로 설정합니다.
	ADSCamera->bUsePawnControlRotation = false;

	// 평소(비조준 상태)에는 꺼져 있어야 하므로 기본 활성화 상태를 false로 둡니다.
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

	// 1. 카메라 이동 로직 수정 (카메라는 고정, FOV만 줌인)
	if (FollowCamera && ADSCamera) // CameraBoom 대신 ADSCamera 확인
	{
		float TargetFOV = bIsAiming ? AimFOV : DefaultFOV;

		// 조준 중일 때는 ADSCamera의 FOV를 좁히고, 아닐 때는 FollowCamera의 FOV를 넓힙니다.
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

	// 탄 퍼짐 수치 서서히 회복
	if (CurrentSpread > 0.0f)
	{
		CurrentSpread = FMath::FInterpTo(CurrentSpread, 0.0f, DeltaTime, SpreadRecoveryRate);
	}

	// --- 절차적 조준(Procedural ADS) 뼈대 타겟 좌표 계산 ---
	if (bIsAiming && FollowCamera && WeaponMesh)
	{
		// 1. 가늠좌(Sight)가 위치해야 할 화면 정중앙의 월드 좌표
		FVector CameraLocation = FollowCamera->GetComponentLocation();
		FVector CameraForward = FollowCamera->GetForwardVector();
		FVector TargetSightWorldLoc = CameraLocation + (CameraForward * AimDistance);

		// 2. 가늠좌에서 무기 손잡이(오른손)까지의 고정된 오프셋 벡터 구하기
		// 무기 메쉬의 루트 위치 = 오른손이 쥐고 있는 위치입니다.
		FVector HandWorldLoc = WeaponMesh->GetComponentLocation();
		FVector SightWorldLoc = WeaponMesh->GetSocketLocation(FName("SightSocket"));
		FVector SightToHandOffset = HandWorldLoc - SightWorldLoc;

		// 3. 목표 가늠좌 위치에 오프셋을 더해 '오른손이 도달해야 할 최종 월드 좌표' 산출
		FVector TargetHandWorldLoc = TargetSightWorldLoc + SightToHandOffset;

		// 4. 캐릭터 기준(Component Space)으로 변환하여 AnimBP가 이해할 수 있게 함
		FTransform MeshTransform = GetMesh()->GetComponentTransform();
		FVector TargetHandCS = MeshTransform.InverseTransformPosition(TargetHandWorldLoc);

		// 5. 프레임 제한 해제 시에도 안전하도록 Interp 적용 (기존에 잘 짜두신 부분 활용)
		DynamicAimOffset = FMath::VInterpTo(DynamicAimOffset, TargetHandCS, DeltaTime, 20.0f);
	}
	// else문에서 ZeroVector로 되돌리는 로직은 삭제하세요. 
	// (0,0,0)은 캐릭터의 발밑이기 때문에, 조준을 풀 때 AnimBP의 Alpha 값으로 조절하는 것이 맞습니다.
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

	// 슈팅 게임 필수 로직: 조준할 때는 캐릭터가 마우스 방향을 바라보며 걷게 만듭니다.
	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;

	// --- [새 코드 추가] 카메라 전환: 3인칭 끄기, 1인칭(가늠좌) 켜기 ---
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

	// 조준을 풀면 다시 자유롭게 뛰어다니도록 원래 상태로 되돌립니다.
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	// --- [새 코드 추가] 카메라 전환: 1인칭(가늠좌) 끄기, 3인칭 켜기 ---
	if (FollowCamera && ADSCamera)
	{
		ADSCamera->SetActive(false);
		FollowCamera->SetActive(true);
	}
}

// 진짜 사격 로직 (Line Trace)
void AGun_phiriaCharacter::Fire()
{
	if (!FollowCamera) return;

	// 사격 자세 켜기 & 0.2초 뒤에 끄도록 타이머 설정
	bIsFiring = true;
	GetWorldTimerManager().SetTimer(FireTimerHandle, this, &AGun_phiriaCharacter::StopFiringPose, 1.0f, false);

	// 조준 여부에 따른 반동/퍼짐 감소 배수 설정
	// 조준 중(bIsAiming이 true)이면 0.3 (30% 수준으로 감소), 아니면 1.0 (100% 원본 그대로)
	float AimMultiplier = bIsAiming ? 0.3f : 1.0f;

	// 1. 물리적 카메라 반동 (화면 튀기)
	if (Controller != nullptr)
	{
		// 원래 랜덤 값에 AimMultiplier를 곱해서 조준 시 반동을 확 줄여줍니다!
		float RecoilPitch = FMath::RandRange(-0.5f, -1.0f) * AimMultiplier;
		float RecoilYaw = FMath::RandRange(-0.5f, 0.5f) * AimMultiplier;

		AddControllerPitchInput(RecoilPitch);
		AddControllerYawInput(RecoilYaw);
	}

	// 2. 탄 퍼짐(Spread) 수치 증가
	// 늘어나는 퍼짐 수치(SpreadPerShot)에도 AimMultiplier를 곱해줍니다.
	CurrentSpread = FMath::Clamp(CurrentSpread + (SpreadPerShot * AimMultiplier), 0.0f, MaxSpread);

	// 1. 카메라의 현재 위치와 바라보는 방향(정중앙 크로스헤어 방향)을 가져옵니다.
	FVector StartLocation = FollowCamera->GetComponentLocation();
	FVector ForwardVector = FollowCamera->GetForwardVector();

	// 2. 사거리 설정 (예: 50미터 = 5000 유닛)
	float TraceDistance = 5000.0f;

	// 시작점에서 앞으로 5000만큼 나아간 끝 지점 계산
	FVector EndLocation = StartLocation + (ForwardVector * TraceDistance);

	// 3. 충돌 결과를 저장할 구조체
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this); // 아주 중요: 내가 쏜 총알에 내 뒤통수가 맞는 걸 방지!

	// 4. 보이지 않는 레이저(Line Trace) 발사!
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		StartLocation,
		EndLocation,
		ECC_Visibility, // 시야(Visibility)에 걸리는 모든 물체와 충돌
		QueryParams
	);

	// 5. 디버그 선으로 총알 궤적 그리기
	if (bHit)
	{
		// 어딘가에 맞았다면: 총구부터 맞은 곳(ImpactPoint)까지만 빨간 선을 그립니다.
		DrawDebugLine(GetWorld(), StartLocation, HitResult.ImpactPoint, FColor::Red, false, 2.0f, 0, 2.0f);

		// 화면 왼쪽 위에 명중한 물체의 이름을 띄워줍니다.
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, FString::Printf(TEXT("명중: %s"), *HitResult.GetActor()->GetName()));
	}
	else
	{
		// 아무것도 안 맞았다면: 허공을 가르는 초록색 선을 그립니다.
		DrawDebugLine(GetWorld(), StartLocation, EndLocation, FColor::Green, false, 2.0f, 0, 2.0f);
	}
}

// 0.2초 뒤에 사격 자세를 푸는 함수
void AGun_phiriaCharacter::StopFiringPose()
{
	bIsFiring = false;
}