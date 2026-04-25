#include "ThrowableWeapon.h"
#include "GrenadeProjectile.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "../Gun_phiriaCharacter.h"
#include "../Component/InventoryComponent.h"

AThrowableWeapon::AThrowableWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	MaxCookTime = 5.0f;
	ThrowSpeed = 1500.0f;
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
			Player->PlayAnimMontage(ThrowMontage, 0.6f, FName("Start"));

			FName CurrentThrowableID = NAME_None;
			if (Player->PlayerInventory)
			{
				CurrentThrowableID = Player->PlayerInventory->EquippedThrowableID;
			}

			Player->StartCasting(MaxCookTime, CurrentThrowableID, [this]() {
				ExplodeInHand();
				}, true, 2.0f);
		}
	}
}

void AThrowableWeapon::ReleaseThrow(FVector StartLocation, FVector ThrowDirection)
{
	if (!bIsCooking || CurrentAmmo <= 0) return;

	bIsCooking = false;

	float ElapsedTime = GetWorld()->GetTimeSeconds() - CookStartTime;
	float RemainingTime = FMath::Max(0.1f, MaxCookTime - ElapsedTime);

	if (AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwner()))
	{
		Player->CancelCasting();

		if (ThrowMontage)
		{
			Player->PlayAnimMontage(ThrowMontage, 1.0f, FName("Throw"));
		}
	}

	if (ProjectileClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetOwner();
		SpawnParams.Instigator = Cast<APawn>(GetOwner());
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		FVector SpawnLocation = StartLocation + (ThrowDirection * 150.0f);
		if (AGrenadeProjectile* Projectile = GetWorld()->SpawnActor<AGrenadeProjectile>(ProjectileClass, SpawnLocation, ThrowDirection.Rotation(), SpawnParams))
		{
			Projectile->InitializeThrow(ThrowDirection * ThrowSpeed, RemainingTime);
		}
	}

	CurrentAmmo--;

	if (CurrentAmmo <= 0)
	{
		if (WeaponMesh) WeaponMesh->SetVisibility(false);
		GetWorldTimerManager().SetTimer(SwapTimerHandle, this, &AThrowableWeapon::ExecutePostThrowSwap, 1.0f, false);
	}
}

void AThrowableWeapon::ExecutePostThrowSwap()
{
	// 중복 로직 통합
	CleanupAndDestroy();
}

void AThrowableWeapon::CancelThrow()
{
	if (!bIsCooking) return;

	bIsCooking = false;

	// [보완] 사용되지 않는 타이머 Clear 대신 캐릭터의 캐스팅을 명시적으로 취소합니다.
	if (AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwner()))
	{
		Player->CancelCasting();
	}
}

void AThrowableWeapon::ExplodeInHand()
{
	bIsCooking = false;

	if (ProjectileClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetOwner();
		SpawnParams.Instigator = Cast<APawn>(GetOwner());
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		FVector HandLocation = WeaponMesh ? WeaponMesh->GetComponentLocation() : GetActorLocation();

		if (AGrenadeProjectile* Projectile = GetWorld()->SpawnActor<AGrenadeProjectile>(ProjectileClass, HandLocation, FRotator::ZeroRotator, SpawnParams))
		{
			Projectile->InitializeThrow(FVector::ZeroVector, 0.01f);
		}
	}

	CurrentAmmo--;
	if (WeaponMesh) WeaponMesh->SetVisibility(false);

	// 중복 로직 통합
	CleanupAndDestroy();
}

void AThrowableWeapon::CleanupAndDestroy()
{
	if (AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwner()))
	{
		if (UInventoryComponent* PlayerInv = Player->PlayerInventory)
		{
			PlayerInv->EquippedThrowableID = NAME_None;
		}

		if (Player->WeaponSlots.IsValidIndex(1) && Player->WeaponSlots[1] != nullptr)
		{
			Player->EquipWeaponSlot(1);
		}
		else
		{
			Player->EquipWeaponSlot(0);
		}

		if (Player->WeaponSlots.IsValidIndex(3))
		{
			Player->WeaponSlots[3] = nullptr;
		}
	}

	this->Destroy();
}