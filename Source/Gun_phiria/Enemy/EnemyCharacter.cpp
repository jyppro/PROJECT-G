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

			// 일반 적 블랙보드에 쿨다운 전달
			if (AAIController* AICon = Cast<AAIController>(GetController()))
			{
				if (UBlackboardComponent* BB = AICon->GetBlackboardComponent())
				{
					if (CurrentWeapon->FireRate > 0.0f)
					{
						float FireDelay = 60.0f / CurrentWeapon->FireRate;
						BB->SetValueAsFloat(FName("AttackCooldown"), FireDelay);
					}
					else
					{
						BB->SetValueAsFloat(FName("AttackCooldown"), 1.5f); // 기본 딜레이
					}
				}
			}
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

	// 상태에 따른 시선 및 이동 회전 처리
	if (bIsAiming)
	{
		// 전투 중에는 이동 방향이 아닌, 플레이어 방향으로 상체를 고정합니다.
		if (GetCharacterMovement())
		{
			GetCharacterMovement()->bOrientRotationToMovement = false;
		}

		if (TObjectPtr<ACharacter> PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0))
		{
			const FVector StartLoc = GetActorLocation();
			const FVector TargetLoc = PlayerCharacter->GetActorLocation();
			const FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation(StartLoc, TargetLoc);

			const FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(LookAtRot, GetActorRotation());
			AimPitch = FMath::FInterpTo(AimPitch, DeltaRot.Pitch, DeltaTime, 15.0f);
			AimPitch = FMath::Clamp(AimPitch, -90.0f, 90.0f);

			const FRotator TargetRotation = FRotator(GetActorRotation().Pitch, LookAtRot.Yaw, GetActorRotation().Roll);
			const FRotator SmoothRotation = FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, 5.0f);
			SetActorRotation(SmoothRotation);
		}
	}
	else
	{
		// 평화 상태(순찰 중)일 때는 걸어가는 방향을 바라봅니다.
		if (GetCharacterMovement())
		{
			GetCharacterMovement()->bOrientRotationToMovement = true;
		}

		// 조준 각도는 정면(0)으로 부드럽게 되돌립니다.
		AimPitch = FMath::FInterpTo(AimPitch, 0.0f, DeltaTime, 5.0f);
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

	// 헤드샷 판정 (대미지 2.5배)
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FPointDamageEvent* PointDamageEvent = static_cast<const FPointDamageEvent*>(&DamageEvent);
		if (PointDamageEvent->HitInfo.BoneName == FName("head"))
		{
			ActualDamage *= 2.5f;
		}
	}

	ActualDamage = Super::TakeDamage(ActualDamage, DamageEvent, EventInstigator, DamageCauser);

	// 실제로 체력을 차감하는 핵심 로직 복구!
	CurrentHealth = FMath::Clamp(CurrentHealth - ActualDamage, 0.0f, MaxHealth);

	// 아직 살아있다면 피격 반응 및 반격 개시
	if (CurrentHealth > 0.0f)
	{
		// 피격 몽타주 재생
		if (HitMontages.Num() > 0)
		{
			const int32 RandomIndex = FMath::RandRange(0, HitMontages.Num() - 1);
			PlayAnimMontage(HitMontages[RandomIndex]);
		}

		// 공격받으면 즉시 전투 상태(조준)로 전환
		bIsAiming = true;

		// 블랙보드에 플레이어를 타겟으로 지정하여 추적 시작
		if (AAIController* AICon = Cast<AAIController>(GetController()))
		{
			if (UBlackboardComponent* BB = AICon->GetBlackboardComponent())
			{
				ACharacter* PlayerChar = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
				BB->SetValueAsObject(FName("TargetActor"), PlayerChar);
			}
		}

		// 체력이 절반 이하일 때의 추가 패턴용 블랙보드 업데이트
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
	}
	// 체력이 0 이하라면 사망 처리
	else
	{
		Die(EventInstigator);
	}

	return ActualDamage;
}

void AEnemyCharacter::Die(AController* Killer)
{
	if (bIsDead) return;
	bIsDead = true;

	// 1. 플레이어에게 골드 보상 지급
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