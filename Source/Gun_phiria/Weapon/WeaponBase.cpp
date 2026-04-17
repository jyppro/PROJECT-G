#include "WeaponBase.h"

// Engine Headers
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "Components/StaticMeshComponent.h"

AWeaponBase::AWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("RootComp"));
	RootComponent = RootComp;

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(RootComponent); // 이제 총기 메시는 뿌리 아래에 붙습니다.
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AWeaponBase::Fire(FVector TargetLocation)
{
	if (!WeaponMesh) return;

	// 1. 탄약 체크 (총알이 없으면 사격 불가)
	if (!bInfiniteAmmo)
	{
		if (CurrentAmmo <= 0)
		{
			// (찰칵 소리 재생 로직 추가)
			if (EmptyMagSound) UGameplayStatics::PlaySoundAtLocation(this, EmptyMagSound, GetActorLocation());
			return;
		}
		CurrentAmmo--;
	}

	// TargetLocation이 ZeroVector면 사격 로직을 멈춤 (총알 없을 때 소리만 나게끔 처리용)
	if (TargetLocation.IsNearlyZero()) return;

	if (ShellEjectEffect && WeaponMesh)
	{
		// 소켓의 위치와 회전값을 가져옵니다.
		FTransform SocketTransform = WeaponMesh->GetSocketTransform(EjectSocketName);

		// 해당 위치에 나이아가라 파티클을 1회성으로 스폰합니다.
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			ShellEjectEffect,
			SocketTransform.GetLocation(),
			SocketTransform.GetRotation().Rotator()
		);
	}

	// 2. 기존 히트스캔 및 나이아가라 로직
	const FVector MuzzleLocation = WeaponMesh->GetSocketLocation(FName("MuzzleSocket"));
	const FVector BaseDirection = (TargetLocation - MuzzleLocation).GetSafeNormal();
	const FVector BulletEndLocation = MuzzleLocation + (BaseDirection * 5000.0f);

	FHitResult BulletHit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(GetOwner());

	const bool bHit = GetWorld()->LineTraceSingleByChannel(BulletHit, MuzzleLocation, BulletEndLocation, ECC_Visibility, QueryParams);

	// 1. Muzzle Flash & Tracer
	if (MuzzleFlashEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(MuzzleFlashEffect, WeaponMesh, FName("MuzzleSocket"), FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true);
	}

	if (BulletTracerEffect)
	{
		if (TObjectPtr<UNiagaraComponent> Tracer = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), BulletTracerEffect, MuzzleLocation))
		{
			const FVector TracerEnd = bHit ? BulletHit.ImpactPoint : BulletEndLocation;
			Tracer->SetVectorParameter(FName("Target"), TracerEnd);
		}
	}

	// 2. Hit Processing
	if (bHit)
	{
		TObjectPtr<AActor> HitActor = BulletHit.GetActor();
		const FRotator ImpactRotation = BulletHit.ImpactNormal.Rotation();

		if (HitActor && Cast<ACharacter>(HitActor))
		{
			// Damage Calculation (Rounded to Integer)
			const float RawDamage = FMath::RandRange(MinWeaponDamage, MaxWeaponDamage);
			const float FinalDamage = static_cast<float>(FMath::RoundToInt(RawDamage));

			UGameplayStatics::ApplyPointDamage(
				HitActor,
				FinalDamage,
				BaseDirection,
				BulletHit,
				GetInstigatorController(),
				this,
				UDamageType::StaticClass()
			);

			// Enemy Hit Effects
			if (EnemyHitEffect)
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), EnemyHitEffect, BulletHit.ImpactPoint, ImpactRotation);
			}
		}
		else if (ImpactEffect)
		{
			// Environmental Impact
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ImpactEffect, BulletHit.ImpactPoint, ImpactRotation);
		}
	}
}