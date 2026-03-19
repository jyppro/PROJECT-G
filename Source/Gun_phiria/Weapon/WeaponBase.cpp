// WeaponBase.cpp
#include "WeaponBase.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"

AWeaponBase::AWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	RootComponent = WeaponMesh;
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

// 기존 캐릭터의 Fire 함수 뒷부분(라인 트레이스 및 이펙트)이 이곳으로 옵니다.
void AWeaponBase::Fire(FVector TargetLocation)
{
	FVector MuzzleLocation = WeaponMesh->GetSocketLocation(FName("MuzzleSocket"));
	FVector BaseDirection = (TargetLocation - MuzzleLocation).GetSafeNormal();

	float TraceDistance = 5000.0f;
	FVector BulletEndLocation = MuzzleLocation + (BaseDirection * TraceDistance);

	FHitResult BulletHit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(GetOwner()); // 무기를 든 주인을 무시

	bool bBulletHit = GetWorld()->LineTraceSingleByChannel(BulletHit, MuzzleLocation, BulletEndLocation, ECC_Visibility, QueryParams);

	// 이펙트 생성 (작성해주신 코드와 동일)
	if (MuzzleFlashEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(MuzzleFlashEffect, WeaponMesh, FName("MuzzleSocket"), FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true);
	}

	if (BulletTracerEffect)
	{
		UNiagaraComponent* TracerComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), BulletTracerEffect, MuzzleLocation);
		if (TracerComponent)
		{
			FVector TracerEnd = bBulletHit ? BulletHit.ImpactPoint : BulletEndLocation;
			TracerComponent->SetVectorParameter(FName("Target"), TracerEnd);
		}
	}

	if (bBulletHit && ImpactEffect)
	{
		FRotator ImpactRotation = BulletHit.ImpactNormal.Rotation();
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ImpactEffect, BulletHit.ImpactPoint, ImpactRotation);
	}
}