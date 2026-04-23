#include "ThrowableWeapon.h"
#include "GrenadeProjectile.h" // 발사체 헤더 포함
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "../Gun_phiriaCharacter.h"
#include "../Component/InventoryComponent.h"

AThrowableWeapon::AThrowableWeapon()
{
	// 투척 무기는 총알이 아니므로 틱(Tick)이 필요 없습니다.
	PrimaryActorTick.bCanEverTick = false;

	MaxCookTime = 5.0f; // 수류탄 핀 뽑고 터지기까지 5초
	ThrowSpeed = 1500.0f; // 던지는 힘
}

void AThrowableWeapon::BeginPlay()
{
	Super::BeginPlay();
}

// 1. 좌클릭 누를 때 (핀 뽑기 & 쿠킹 시작)
void AThrowableWeapon::Fire(FVector TargetLocation)
{
	if (bIsCooking || CurrentAmmo <= 0) return;

	bIsCooking = true;
	CookStartTime = GetWorld()->GetTimeSeconds();

	if (FireSound) UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());

	GetWorldTimerManager().SetTimer(CookTimerHandle, this, &AThrowableWeapon::ExplodeInHand, MaxCookTime, false);

	// [애니메이션 1] 핀을 뽑거나 팔을 뒤로 젖히는 쿠킹 몽타주 재생
	// (헤더에 UAnimMontage* CookMontage; 를 추가해 주시면 더 완벽합니다)
	if (ThrowMontage)
	{
		if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
			OwnerCharacter->PlayAnimMontage(ThrowMontage);
	}
}

void AThrowableWeapon::ReleaseThrow(FVector StartLocation, FVector ThrowDirection)
{
	if (!bIsCooking || CurrentAmmo <= 0) return;

	bIsCooking = false;
	GetWorldTimerManager().ClearTimer(CookTimerHandle);

	float ElapsedTime = GetWorld()->GetTimeSeconds() - CookStartTime;
	float RemainingTime = FMath::Max(0.1f, MaxCookTime - ElapsedTime);

	// 발사체 스폰
	if (ProjectileClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetOwner();
		SpawnParams.Instigator = Cast<APawn>(GetOwner());

		// [겹침 버그 해결 핵심] 몸통에 겹쳐도 무조건 스폰해라! ( AlwaysSpawn )
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AGrenadeProjectile* Projectile = GetWorld()->SpawnActor<AGrenadeProjectile>(ProjectileClass, StartLocation, ThrowDirection.Rotation(), SpawnParams);

		if (Projectile)
		{
			Projectile->InitializeThrow(ThrowDirection * ThrowSpeed, RemainingTime);
		}
	}

	// [수량 미감소 해결 핵심] 플레이어 인벤토리에서 실제 아이템 1개를 삭제합니다.
	if (AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwner()))
	{
		if (UInventoryComponent* PlayerInv = Player->PlayerInventory)
		{
			// [핵심] 던졌으므로 장착 슬롯을 비워줍니다.
			// 그래야 다시 가방에서 다음 수류탄을 장착할 수 있습니다.
			PlayerInv->EquippedThrowableID = NAME_None;
		}
	}

	CurrentAmmo--; // 손에 든 수량 1 -> 0

	if (CurrentAmmo <= 0)
	{
		if (WeaponMesh) WeaponMesh->SetVisibility(false);

		if (AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwner()))
		{
			// 1. 주무기1로 먼저 자동 스왑
			Player->EquipWeaponSlot(1);

			if (Player->WeaponSlots.IsValidIndex(3))
			{
				Player->WeaponSlots[3] = nullptr;
			}
			this->Destroy(); // 나 자신(빈 수류탄)을 월드에서 삭제
		}
	}
}

// 3. 수류탄 핀을 뽑은 채로 다른 무기로 스왑할 때 (취소)
void AThrowableWeapon::CancelThrow()
{
	if (!bIsCooking) return;

	// (참고: 진짜 배그 하드코어 모드라면 핀 뽑은 걸 취소 못하고 발밑에 떨어뜨려야 합니다.
	// 우선은 편의상 타이머를 끄고 쿠킹을 취소하는 로직으로 둡니다.)
	bIsCooking = false;
	GetWorldTimerManager().ClearTimer(CookTimerHandle);
}

// 4. 타이머 오버 (손에서 쾅!)
void AThrowableWeapon::ExplodeInHand()
{
	bIsCooking = false;

	if (ProjectileClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetOwner();
		SpawnParams.Instigator = Cast<APawn>(GetOwner());
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		// 손 위치에서 스폰
		FVector HandLocation = WeaponMesh ? WeaponMesh->GetComponentLocation() : GetActorLocation();

		AGrenadeProjectile* Projectile = GetWorld()->SpawnActor<AGrenadeProjectile>(ProjectileClass, HandLocation, FRotator::ZeroRotator, SpawnParams);

		if (Projectile)
		{
			// =======================================================
			// [핵심] 속도는 0으로 주고, 타이머는 0이 아닌 아주 짧은 시간(0.01초)을 주어 즉시 폭발하게 만듭니다!
			// =======================================================
			Projectile->InitializeThrow(FVector::ZeroVector, 0.01f);
		}
	}

	CurrentAmmo--;

	// =======================================================
	// [후처리] 손에서 터졌을 때도 '던졌을 때'와 똑같이 슬롯을 비우고 스왑해야 합니다.
	// =======================================================
	if (WeaponMesh) WeaponMesh->SetVisibility(false);

	if (AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwner()))
	{
		// 1. 장착 슬롯 비우기 (다시 장착할 수 있도록)
		if (UInventoryComponent* PlayerInv = Player->PlayerInventory)
		{
			PlayerInv->EquippedThrowableID = NAME_None;
		}

		// 2. 주무기(1번)로 강제 스왑
		Player->EquipWeaponSlot(1);

		// 3. 껍데기 파괴
		if (Player->WeaponSlots.IsValidIndex(3))
		{
			Player->WeaponSlots[3] = nullptr;
		}
		this->Destroy();
	}
}