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

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	RootComponent = WeaponMesh;
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AWeaponBase::Fire(FVector TargetLocation)
{
	if (!WeaponMesh) return;

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