#include "EnemyCharacter.h"
#include "../Weapon/WeaponBase.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "Engine/DamageEvents.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"

AEnemyCharacter::AEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// ★ [콜리전 자동화]
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);

	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}

void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 시작할 때 현재 체력을 최대 체력으로 꽉 채웁니다.
	CurrentHealth = MaxHealth;

	// 1. 무기 스폰 및 장착 (플레이어와 완벽히 동일한 로직입니다)
	if (DefaultWeaponClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		CurrentWeapon = GetWorld()->SpawnActor<AWeaponBase>(DefaultWeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

		if (CurrentWeapon)
		{
			FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
			// 적 캐릭터의 스켈레탈 메시 중 "WeaponSocket"에 무기를 부착합니다.
			CurrentWeapon->AttachToComponent(GetMesh(), AttachmentRules, FName("WeaponSocket"));
		}
	}

	// 2. 게임이 시작되면 FireRate(예: 2초) 간격으로 FireAtPlayer 함수를 무한 반복 실행합니다.
	GetWorldTimerManager().SetTimer(AIFireTimer, this, &AEnemyCharacter::FireAtPlayer, FireRate, true);
}

void AEnemyCharacter::FireAtPlayer()
{
	// 무기가 없거나 이미 죽었다면 쏘지 않음
	if (!CurrentWeapon || bIsDead) return;

	// 월드에 존재하는 플레이어 캐릭터를 찾아옵니다.
	ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);

	if (PlayerCharacter)
	{
		// 플레이어의 현재 정중앙 위치를 타겟 좌표로 삼습니다.
		FVector TargetLocation = PlayerCharacter->GetActorLocation();

		// ==========================================================
		// ★ [새로 추가된 부분] 적의 조준에 오차(Inaccuracy)를 줍니다!
		// 이 수치(80.0f)를 키우면 적이 멍청해지고, 줄이면 명사수가 됩니다.
		float Inaccuracy = 80.0f;

		float RandomX = FMath::RandRange(-Inaccuracy, Inaccuracy);
		float RandomY = FMath::RandRange(-Inaccuracy, Inaccuracy);
		float RandomZ = FMath::RandRange(-Inaccuracy, Inaccuracy);

		// 정확한 타겟 좌표에 랜덤한 오차를 더해 조준을 흐트러뜨립니다.
		TargetLocation += FVector(RandomX, RandomY, RandomZ);
		// ==========================================================

		// 내 손에 들려있는 무기에게 '오차가 적용된 엉뚱한 위치'를 향해 쏘라고 명령합니다!
		CurrentWeapon->Fire(TargetLocation);
	}
}

float AEnemyCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead) return 0.0f;

	float ActualDamage = DamageAmount;

	// =========================================================
	// ★ [헤드샷 판별 로직] 데미지 종류가 PointDamage인지 확인합니다.
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		// 이벤트를 포인트 데미지로 변환해서 정보를 꺼냅니다.
		const FPointDamageEvent* PointDamageEvent = static_cast<const FPointDamageEvent*>(&DamageEvent);
		FName HitBoneName = PointDamageEvent->HitInfo.BoneName;

		// 뼈 이름이 "head"인지 확인합니다. (마네킹 기본 머리 뼈 이름은 보통 "head"입니다)
		if (HitBoneName == FName("head"))
		{
			ActualDamage *= 2.5f; // 헤드샷 2.5배 데미지!

			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("HEADSHOT! Critical Damage!"));
			}
		}
	}
	// =========================================================

	// 부모 클래스의 기본 데미지 처리 로직 실행
	ActualDamage = Super::TakeDamage(ActualDamage, DamageEvent, EventInstigator, DamageCauser);

	// 체력을 깎고 안전장치(Clamp) 적용
	CurrentHealth -= ActualDamage;
	CurrentHealth = FMath::Clamp(CurrentHealth, 0.0f, MaxHealth);

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

// 3. 사망 처리 함수 구현
void AEnemyCharacter::Die()
{
	if (bIsDead) return;
	bIsDead = true;

	// 1. 더 이상 총을 쏘지 못하도록 타이머를 멈춥니다.
	GetWorldTimerManager().ClearTimer(AIFireTimer);

	// 2. 들고 있던 무기 파괴
	if (CurrentWeapon)
	{
		CurrentWeapon->Destroy();
	}

	// 3. 충돌체(캡슐)를 꺼서 플레이어가 지나갈 수 있게 만듭니다.
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 4. (선택사항) 래그돌(물리 엔진으로 쓰러짐) 적용
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	GetMesh()->SetSimulatePhysics(true);

	// 5. 시체가 너무 많이 쌓이지 않도록 5초 뒤에 액터 완전 삭제
	SetLifeSpan(5.0f);
}