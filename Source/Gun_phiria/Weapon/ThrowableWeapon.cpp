#include "ThrowableWeapon.h"
#include "GrenadeProjectile.h" // 발사체 헤더 포함
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "../Gun_phiriaCharacter.h"

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

// ==============================================================
// 1. 좌클릭 누를 때 (핀 뽑기 & 쿠킹 시작)
// ==============================================================
void AThrowableWeapon::Fire(FVector TargetLocation)
{
	// 이미 핀을 뽑았거나 수류탄이 없으면 무시
	if (bIsCooking || CurrentAmmo <= 0) return;

	bIsCooking = true;

	// 핀을 뽑은 '현재 시간'을 기록합니다.
	CookStartTime = GetWorld()->GetTimeSeconds();

	// 핀 뽑는 소리 재생 (찰칵!)
	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}

	// 5초(MaxCookTime) 뒤에 손에서 터지도록 자폭 타이머 가동!
	GetWorldTimerManager().SetTimer(CookTimerHandle, this, &AThrowableWeapon::ExplodeInHand, MaxCookTime, false);

	// 쿠킹 애니메이션 재생 (선택)
	if (ThrowMontage)
	{
		ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
		if (OwnerCharacter) OwnerCharacter->PlayAnimMontage(ThrowMontage);
	}
}

// 2. 좌클릭 뗄 때 (투척 실행)
void AThrowableWeapon::ReleaseThrow(FVector StartLocation, FVector ThrowDirection)
{
	if (!bIsCooking || CurrentAmmo <= 0) return;

	bIsCooking = false;
	GetWorldTimerManager().ClearTimer(CookTimerHandle);

	float ElapsedTime = GetWorld()->GetTimeSeconds() - CookStartTime;
	float RemainingTime = FMath::Max(0.1f, MaxCookTime - ElapsedTime);

	if (ProjectileClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetOwner();
		SpawnParams.Instigator = Cast<APawn>(GetOwner());

		// [해결책] 무조건 스폰시키고, 위치를 손에 든 무기 메시 위치에서 시작하게 함
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		// 카메라 위치가 아닌, 현재 손에 들고 있는 수류탄의 위치를 사용합니다.
		FVector SpawnLocation = WeaponMesh ? WeaponMesh->GetComponentLocation() : StartLocation;

		AGrenadeProjectile* Projectile = GetWorld()->SpawnActor<AGrenadeProjectile>(ProjectileClass, SpawnLocation, ThrowDirection.Rotation(), SpawnParams);

		if (Projectile)
		{
			Projectile->InitializeThrow(ThrowDirection * ThrowSpeed, RemainingTime);
		}
	}

	CurrentAmmo--;

	// 수류탄을 다 던지면 손에서 안 보이게 처리
	if (CurrentAmmo <= 0 && WeaponMesh)
	{
		WeaponMesh->SetVisibility(false);

		// [추가] 다 던졌으면 다시 주무기(1번)로 자동으로 바꾸게 하면 편리합니다.
		if (AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwner()))
		{
			Player->EquipWeaponSlot(1);
		}
	}
}

// ==============================================================
// 3. 수류탄 핀을 뽑은 채로 다른 무기로 스왑할 때 (취소)
// ==============================================================
void AThrowableWeapon::CancelThrow()
{
	if (!bIsCooking) return;

	// (참고: 진짜 배그 하드코어 모드라면 핀 뽑은 걸 취소 못하고 발밑에 떨어뜨려야 합니다.
	// 우선은 편의상 타이머를 끄고 쿠킹을 취소하는 로직으로 둡니다.)
	bIsCooking = false;
	GetWorldTimerManager().ClearTimer(CookTimerHandle);
}

// ==============================================================
// 4. 타이머 오버 (손에서 쾅!)
// ==============================================================
void AThrowableWeapon::ExplodeInHand()
{
	bIsCooking = false;

	// 플레이어 위치에 발사체를 생성하고 남은 시간을 0으로 줘서 즉시 폭발시킵니다!
	if (ProjectileClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetOwner();
		SpawnParams.Instigator = Cast<APawn>(GetOwner());

	FVector HandLocation = WeaponMesh->GetComponentLocation();

	AGrenadeProjectile* Projectile = GetWorld()->SpawnActor<AGrenadeProjectile>(ProjectileClass, HandLocation, FRotator::ZeroRotator, SpawnParams);

		if (Projectile)
		{
			// 속도 0, 남은 시간 0 = 손에서 즉사
			Projectile->InitializeThrow(FVector::ZeroVector, 0.0f);
		}
	}

	CurrentAmmo--;
	if (CurrentAmmo <= 0 && WeaponMesh)
	{
		WeaponMesh->SetVisibility(false);
	}
}