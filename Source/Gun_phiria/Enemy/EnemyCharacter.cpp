#include "EnemyCharacter.h"
#include "../Weapon/WeaponBase.h"
#include "../ProceduralDungeonGeneration/DungeonRoomManager.h"
#include "../Gun_phiriaCharacter.h"

// Engine Headers
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "TimerManager.h"
#include "Engine/DamageEvents.h"

// Component Headers
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

// AI Headers
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

AEnemyCharacter::AEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Collision Setup
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);

	if (TObjectPtr<USkeletalMeshComponent> MeshComp = GetMesh())
	{
		MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		MeshComp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	}

	// Movement Setup
	if (TObjectPtr<UCharacterMovementComponent> MoveComp = GetCharacterMovement())
	{
		MoveComp->NavAgentProps.bCanCrouch = true;
		MoveComp->MaxWalkSpeedCrouched = 250.0f;
	}

	MinGoldDrop = 10;
	MaxGoldDrop = 30;
}

void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;
	bIsAiming = false;
	bIsCrouching = false;

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
		}
	}

	// Fire Readiness Timer
	bIsReadyToFire = false;
	GetWorldTimerManager().SetTimer(SpawnDelayTimer, this, &AEnemyCharacter::SetReadyToFire, 1.5f, false);
}

void AEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsDead) return;

	if (TObjectPtr<ACharacter> PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0))
	{
		const FVector StartLoc = GetActorLocation();
		const FVector TargetLoc = PlayerCharacter->GetActorLocation();
		const FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation(StartLoc, TargetLoc);

		// 1. Aim Pitch Calculation (Offset)
		const FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(LookAtRot, GetActorRotation());
		AimPitch = FMath::FInterpTo(AimPitch, DeltaRot.Pitch, DeltaTime, 15.0f);
		AimPitch = FMath::Clamp(AimPitch, -90.0f, 90.0f);

		// 2. Body Rotation (Yaw Interp)
		const FRotator TargetRotation = FRotator(GetActorRotation().Pitch, LookAtRot.Yaw, GetActorRotation().Roll);
		const FRotator SmoothRotation = FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, 5.0f);
		SetActorRotation(SmoothRotation);
	}
}

void AEnemyCharacter::FireAtPlayer()
{
	if (!CurrentWeapon || bIsDead || !bIsReadyToFire) return;

	if (TObjectPtr<ACharacter> PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0))
	{
		FVector TargetLocation = PlayerCharacter->GetActorLocation();

		// Add Inaccuracy
		const float Inaccuracy = 80.0f;
		TargetLocation += FVector(
			FMath::RandRange(-Inaccuracy, Inaccuracy),
			FMath::RandRange(-Inaccuracy, Inaccuracy),
			FMath::RandRange(-Inaccuracy, Inaccuracy)
		);

		CurrentWeapon->Fire(TargetLocation);
	}
}

float AEnemyCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead) return 0.0f;

	float ActualDamage = DamageAmount;

	// Headshot Detection
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FPointDamageEvent* PointDamageEvent = static_cast<const FPointDamageEvent*>(&DamageEvent);
		if (PointDamageEvent->HitInfo.BoneName == FName("head"))
		{
			ActualDamage *= 2.5f;
		}
	}

	ActualDamage = Super::TakeDamage(ActualDamage, DamageEvent, EventInstigator, DamageCauser);

	// Hit Reaction
	if (CurrentHealth > ActualDamage && HitMontages.Num() > 0)
	{
		const int32 RandomIndex = FMath::RandRange(0, HitMontages.Num() - 1);
		PlayAnimMontage(HitMontages[RandomIndex]);
	}

	// Update Health State
	CurrentHealth = FMath::Clamp(CurrentHealth - ActualDamage, 0.0f, MaxHealth);

	// Blackboard Update for Low Health
	if (CurrentHealth <= MaxHealth * 0.5f)
	{
		if (AAIController* AICon = Cast<AAIController>(GetController()))
		{
			if (UBlackboardComponent* BB = AICon->GetBlackboardComponent())
			{
				BB->SetValueAsBool(FName("IsLowHealth"), true);
			}
		}
	}

	if (CurrentHealth <= 0.0f)
	{
		Die(EventInstigator);
	}

	return ActualDamage;
}

// --- 수정된 부분: 매개변수 AController* Killer 추가 ---
void AEnemyCharacter::Die(AController* Killer)
{
	if (bIsDead) return;
	bIsDead = true;

	// ==========================================
	// 1. 플레이어에게 골드 보상 지급
	// ==========================================
	int32 DroppedGold = FMath::RandRange(MinGoldDrop, MaxGoldDrop);

	// 나를 죽인 킬러가 있다면
	if (Killer)
	{
		if (AGun_phiriaCharacter* PlayerChar = Cast<AGun_phiriaCharacter>(Killer->GetPawn()))
		{
			PlayerChar->AddGold(DroppedGold);
		}
	}
	else
	{
		// 폭발통이나 환경 피해로 죽어서 Killer가 nullptr인 경우, 
		// 싱글플레이 기준 0번 플레이어에게 돈을 줍니다. (선택 사항)
		if (AGun_phiriaCharacter* PlayerChar = Cast<AGun_phiriaCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)))
		{
			PlayerChar->AddGold(DroppedGold);
		}
	}
	// ==========================================

	// 2. 무기 파괴 및 물리(랙돌) 처리
	if (CurrentWeapon)
	{
		CurrentWeapon->Destroy();
	}

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	GetMesh()->SetSimulatePhysics(true);

	// 3. 방에 사망 알림 및 시체 소멸 타이머 설정
	if (ParentRoom)
	{
		ParentRoom->OnEnemyDied();
	}

	SetLifeSpan(5.0f);
}

void AEnemyCharacter::SetReadyToFire()
{
	bIsReadyToFire = true;
}