#include "ThrowableWeapon.h"
#include "GrenadeProjectile.h" // 발사체 헤더 포함
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "../Gun_phiriaCharacter.h"
#include "../Component/InventoryComponent.h"
#include "../UI/CastBarWidget.h"

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

void AThrowableWeapon::Fire(FVector TargetLocation)
{
	if (bIsCooking || CurrentAmmo <= 0) return;

	bIsCooking = true;
	CookStartTime = GetWorld()->GetTimeSeconds();

	if (FireSound) UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());

	if (ThrowMontage)
	{
		if (AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwner()))
		{
			// 1. 애니메이션 재생
			Player->PlayAnimMontage(ThrowMontage, 0.6f, FName("Start"));

			// 2. 인벤토리에서 현재 손에 들고 있는 수류탄의 ID를 가져옵니다.
			FName CurrentThrowableID = NAME_None;
			if (Player->PlayerInventory)
			{
				CurrentThrowableID = Player->PlayerInventory->EquippedThrowableID;
			}

			Player->StartCasting(MaxCookTime, CurrentThrowableID, [this]() {
				ExplodeInHand();
				}, true, 2.0f); // 수류탄 전용 빨간색 경고 (2초)
		}
	}
}

void AThrowableWeapon::ReleaseThrow(FVector StartLocation, FVector ThrowDirection)
{
	if (!bIsCooking || CurrentAmmo <= 0) return;

	bIsCooking = false;
	// GetWorldTimerManager().ClearTimer(CookTimerHandle);

	float ElapsedTime = GetWorld()->GetTimeSeconds() - CookStartTime;
	float RemainingTime = FMath::Max(0.1f, MaxCookTime - ElapsedTime);

	// 2. 캐릭터의 캐스팅 시스템 취소 (타이머 종료, UI 숨김, 걷기 속도 복구)
	if (AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwner()))
	{
		Player->CancelCasting();

		if (ThrowMontage)
		{
			Player->PlayAnimMontage(ThrowMontage, 1.0f, FName("Throw"));
		}
	}

	// 발사체(실제 수류탄 액터) 스폰 및 날려보내기
	if (ProjectileClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetOwner();
		SpawnParams.Instigator = Cast<APawn>(GetOwner());
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		FVector SpawnLocation = StartLocation + (ThrowDirection * 150.0f);
		AGrenadeProjectile* Projectile = GetWorld()->SpawnActor<AGrenadeProjectile>(ProjectileClass, SpawnLocation, ThrowDirection.Rotation(), SpawnParams);

		if (Projectile)
		{
			Projectile->InitializeThrow(ThrowDirection * ThrowSpeed, RemainingTime);
		}
	}

	// 손에 쥔 수량 0으로 감소
	CurrentAmmo--;

	// 다 던졌을 때의 후처리 (일회용 시스템)
	if (CurrentAmmo <= 0)
	{
		// 1. 손에 들려있던 수류탄 메시만 즉시 숨겨서 던진 것처럼 연출
		if (WeaponMesh) WeaponMesh->SetVisibility(false);

		// 2. 1초(원하는 대기 시간) 뒤에 스왑 및 삭제 로직 실행
		GetWorldTimerManager().SetTimer(SwapTimerHandle, this, &AThrowableWeapon::ExecutePostThrowSwap, 1.0f, false);
	}
}

void AThrowableWeapon::ExecutePostThrowSwap()
{
	if (AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwner()))
	{
		// 1. 인벤토리 장착 해제
		if (UInventoryComponent* PlayerInv = Player->PlayerInventory)
		{
			PlayerInv->EquippedThrowableID = NAME_None;
		}

		// 2. 주무기(1번 슬롯) 장착 검사 및 스왑
		// WeaponSlots 배열의 1번 인덱스가 유효하고, 실제로 무기 포인터가 들어있다면
		if (Player->WeaponSlots.IsValidIndex(1) && Player->WeaponSlots[1] != nullptr)
		{
			Player->EquipWeaponSlot(1);
		}
		else
		{
			// 1번 무기가 없다면 기본 무기로 스왑 
			// (일반적으로 맨손이나 기본 무기가 0번이라고 가정했습니다. 시스템에 맞게 슬롯 번호를 수정해 주세요!)
			Player->EquipWeaponSlot(0);
		}

		// 3. 다 쓴 수류탄 껍데기 배열에서 비우기
		if (Player->WeaponSlots.IsValidIndex(3))
		{
			Player->WeaponSlots[3] = nullptr;
		}
	}

	// 4. 대기와 스왑이 모두 끝났으므로 수류탄 액터 완전 삭제
	this->Destroy();
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