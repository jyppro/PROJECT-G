#include "GrenadeProjectile.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"

AGrenadeProjectile::AGrenadeProjectile()
{
	// 수류탄 전용 세팅 (필요시)
}

void AGrenadeProjectile::Explode()
{
	// 1. 이펙트 및 사운드 재생
	if (ExplosionEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());
	}
	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, GetActorLocation());
	}

	// 2. 광역 데미지 (Radial Damage) 적용! (배그 수류탄의 핵심)
	TArray<AActor*> IgnoredActors;
	IgnoredActors.Add(this);

	UGameplayStatics::ApplyRadialDamage(
		GetWorld(),
		ExplosionDamage,
		GetActorLocation(),
		ExplosionRadius,
		UDamageType::StaticClass(),
		IgnoredActors,
		this,
		GetInstigatorController(),
		true // 데미지 거리에 따라 감소 여부
	);

	// 3. 폭발 후 자신 삭제
	Destroy();
}