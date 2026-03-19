#include "EnemyCharacter.h"
#include "../Weapon/WeaponBase.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"

AEnemyCharacter::AEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

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
	// 무기가 없으면 쏘지 않습니다.
	if (!CurrentWeapon) return;

	// 월드에 존재하는 플레이어 캐릭터(인덱스 0번)를 찾아옵니다.
	ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);

	if (PlayerCharacter)
	{
		// 플레이어의 현재 몸통 위치를 타겟 좌표로 삼습니다.
		FVector TargetLocation = PlayerCharacter->GetActorLocation();

		// 내 손에 들려있는 무기에게 플레이어 위치를 향해 쏘라고 명령합니다!
		CurrentWeapon->Fire(TargetLocation);
	}
}