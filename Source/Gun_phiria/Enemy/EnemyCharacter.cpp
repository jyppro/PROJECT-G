#include "EnemyCharacter.h"
#include "../Weapon/WeaponBase.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "Engine/DamageEvents.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "../ProceduralDungeonGeneration/DungeonRoomManager.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AEnemyCharacter::AEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// 콜리전 세팅 : 라인 트레이스로 피격 처리를 하기 위함
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);

	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	if (GetCharacterMovement())
	{
		// 숙이기 기능 활성화!
		GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

		// (선택) 숙인 상태로 이동할 때의 최대 속도 조절
		GetCharacterMovement()->MaxWalkSpeedCrouched = 250.0f;
	}
}

void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;

	bIsAiming = false;
	bIsCrouching = false;

	// 무기 스폰 및 장착 (플레이어와 동일)
	if (DefaultWeaponClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		CurrentWeapon = GetWorld()->SpawnActor<AWeaponBase>(DefaultWeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

		if (CurrentWeapon)
		{
			FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);

			// 적 캐릭터의 스켈레탈 메시 중 "WeaponSocket"에 무기를 부착
			CurrentWeapon->AttachToComponent(GetMesh(), AttachmentRules, FName("WeaponSocket"));
		}
	}

	bIsReadyToFire = false;
	GetWorldTimerManager().SetTimer(SpawnDelayTimer, this, &AEnemyCharacter::SetReadyToFire, 1.5f, false);
}

void AEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsDead)
	{
		ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
		if (PlayerCharacter)
		{
			FVector StartLoc = GetActorLocation();
			FVector TargetLoc = PlayerCharacter->GetActorLocation();

			// 플레이어를 향하는 절대적인 회전값을 구합니다.
			FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation(StartLoc, TargetLoc);

			// 1. [상하 조준] 에임 오프셋을 위한 Pitch 계산 (기존 코드 유지)
			FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(LookAtRot, GetActorRotation());
			AimPitch = FMath::FInterpTo(AimPitch, DeltaRot.Pitch, DeltaTime, 15.0f);
			AimPitch = FMath::Clamp(AimPitch, -90.0f, 90.0f);

			// ==========================================================
			// ★ [새로 추가] 2. [좌우 회전] 몸통(Actor) 전체를 플레이어 쪽으로 부드럽게 돌립니다!

			// 현재 내 회전값에서 좌우(Yaw)만 플레이어 방향(LookAtRot.Yaw)으로 바꾼 목표 회전값을 만듭니다.
			FRotator TargetRotation = FRotator(GetActorRotation().Pitch, LookAtRot.Yaw, GetActorRotation().Roll);

			// RInterpTo를 사용해 한 번에 홱! 돌지 않고 자연스럽게 스윽~ 돌아보게 만듭니다. (5.0f는 회전 속도)
			FRotator SmoothRotation = FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, 5.0f);

			// 계산된 부드러운 회전값을 캐릭터에게 적용합니다.
			SetActorRotation(SmoothRotation);
			// ==========================================================
		}
	}
}

void AEnemyCharacter::FireAtPlayer()
{
	// ★ [수정됨] 무기가 없거나, 죽었거나, "아직 쏠 준비가 안 되었다면(!bIsReadyToFire)" 그냥 돌아갑니다(return)!
	if (!CurrentWeapon || bIsDead || !bIsReadyToFire) return;

	// 월드에 존재하는 플레이어 캐릭터를 찾기
	ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);

	if (PlayerCharacter)
	{
		// 플레이어의 현재 정중앙 위치를 타겟 좌표로 삼기
		FVector TargetLocation = PlayerCharacter->GetActorLocation();

		// 적의 조준에 오차 주기 
		// 이 수치(80.0f)를 키우면 적이 멍청해지고, 줄이면 명사수가 됨
		float Inaccuracy = 80.0f;

		float RandomX = FMath::RandRange(-Inaccuracy, Inaccuracy);
		float RandomY = FMath::RandRange(-Inaccuracy, Inaccuracy);
		float RandomZ = FMath::RandRange(-Inaccuracy, Inaccuracy);

		// 정확한 타겟 좌표에 랜덤한 오차를 더해 조준을 흐트러뜨림
		TargetLocation += FVector(RandomX, RandomY, RandomZ);

		// 내 손에 들려있는 무기에게 '오차가 적용된 엉뚱한 위치'를 향해 쏘라고 명령
		CurrentWeapon->Fire(TargetLocation);
	}
}

float AEnemyCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead) return 0.0f;

	float ActualDamage = DamageAmount;

	// [헤드샷 판별 로직] 데미지 종류가 PointDamage인지 확인
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		// 이벤트를 포인트 데미지로 변환해서 정보를 꺼냄
		const FPointDamageEvent* PointDamageEvent = static_cast<const FPointDamageEvent*>(&DamageEvent);
		FName HitBoneName = PointDamageEvent->HitInfo.BoneName;

		// 뼈 이름이 "head"인지 확인 (마네킹 기본 머리 뼈 이름은 보통 "head")
		if (HitBoneName == FName("head"))
		{
			ActualDamage *= 2.5f; // 헤드샷 2.5배 데미지

			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("HEADSHOT! Critical Damage!"));
			}
		}
	}

	// 부모 클래스의 기본 데미지 처리 로직 실행
	ActualDamage = Super::TakeDamage(ActualDamage, DamageEvent, EventInstigator, DamageCauser);

	if (CurrentHealth > ActualDamage)
	{
		// 배열 안에 몽타주가 1개 이상 들어있는지 안전 검사
		if (HitMontages.Num() > 0)
		{
			// 0번부터 (배열 개수 - 1)번 사이의 숫자를 랜덤으로 하나 뽑습니다.
			// (예: 4개가 들어있으면 0, 1, 2, 3 중 하나가 나옴)
			int32 RandomIndex = FMath::RandRange(0, HitMontages.Num() - 1);

			// 당첨된 번호의 몽타주를 재생합니다!
			PlayAnimMontage(HitMontages[RandomIndex]);
		}
	}

	// 체력을 깎고 안전장치(Clamp) 적용
	CurrentHealth -= ActualDamage;
	CurrentHealth = FMath::Clamp(CurrentHealth, 0.0f, MaxHealth);

	if (CurrentHealth <= MaxHealth * 0.5f)
	{
		AAIController* AICon = Cast<AAIController>(GetController());
		if (AICon && AICon->GetBlackboardComponent())
		{
			// 블랙보드의 "IsLowHealth" 라는 키를 True로 만듭니다.
			AICon->GetBlackboardComponent()->SetValueAsBool(FName("IsLowHealth"), true);
		}
	}

	// 디버그용: 맞을 때마다 화면에 체력 표시
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Orange, FString::Printf(TEXT("Enemy Hit! Remaining HP: %.1f (Damage: %.1f)"), CurrentHealth, ActualDamage));
	}

	// 체력이 0이 되면 사망 처리
	if (CurrentHealth <= 0.0f)
	{
		Die();
	}

	return ActualDamage;
}

// 사망 처리 함수 구현
void AEnemyCharacter::Die()
{
	if (bIsDead) return;
	bIsDead = true;

	// 더 이상 총을 쏘지 못하도록 타이머를 멈춤
	// GetWorldTimerManager().ClearTimer(AIFireTimer);

	// 들고 있던 무기 파괴
	if (CurrentWeapon)
	{
		CurrentWeapon->Destroy();
	}

	// 충돌체(캡슐)를 꺼서 플레이어가 지나갈 수 있게 만듦
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 래그돌 적용
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	GetMesh()->SetSimulatePhysics(true);

	if (ParentRoom)
	{
		ParentRoom->OnEnemyDied();
	}

	// 시체가 너무 많이 쌓이지 않도록 5초 뒤에 액터 완전 삭제
	SetLifeSpan(5.0f);
}

// 1.5초 뒤에 이 함수가 실행되면서 방아쇠 잠금이 풀립니다.
void AEnemyCharacter::SetReadyToFire()
{
	bIsReadyToFire = true;
}