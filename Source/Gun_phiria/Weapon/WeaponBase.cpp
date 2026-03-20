#include "WeaponBase.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

AWeaponBase::AWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	RootComponent = WeaponMesh;
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

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

	// 총구 화염 및 총알 궤적 이펙트 스폰
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

	// 총알이 어딘가에 맞았을 때의 처리 (데미지 및 피격 이펙트)
	if (bBulletHit)
	{
		FRotator ImpactRotation = BulletHit.ImpactNormal.Rotation();
		AActor* HitActor = BulletHit.GetActor();

		// 맞은 대상이 캐릭터인지 확인
		if (HitActor && Cast<ACharacter>(HitActor))
		{
			// float로 되어있는 최소/최대 데미지를 정수(int32)로 변환 (반올림)
			int32 MinDamageInt = FMath::RoundToInt(MinWeaponDamage);
			int32 MaxDamageInt = FMath::RoundToInt(MaxWeaponDamage);

			// 정수 기반의 RandRange를 사용하여 10 ~ 20 사이의 깔끔한 정수를 뽑아냄
			int32 RandomIntDamage = FMath::RandRange(MinDamageInt, MaxDamageInt);

			// 데미지 적용 함수(ApplyPointDamage)는 float를 요구하므로 다시 float로 형변환(Cast)
			float RandomDamage = static_cast<float>(RandomIntDamage);

			UGameplayStatics::ApplyPointDamage(
				HitActor,
				RandomDamage, // 이제 14.0f, 17.0f 처럼 소수점 없는 깔끔한 데미지가 들어감
				BaseDirection,
				BulletHit,
				GetInstigatorController(),
				this,
				UDamageType::StaticClass()
			);

			// 플레이어가 쐈을 때만 명중 로그 출력
			APlayerController* PC = Cast<APlayerController>(GetInstigatorController());
			if (PC && GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, FString::Printf(TEXT("Hit Actor: %s"), *HitActor->GetName()));
			}

			// 피 튀김 등 적 전용 이펙트 생성
			if (EnemyHitEffect)
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), EnemyHitEffect, BulletHit.ImpactPoint, ImpactRotation);
			}
		}
		else
		{
			// 벽이나 엄폐물을 맞췄을 때의 일반 이펙트
			if (ImpactEffect)
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ImpactEffect, BulletHit.ImpactPoint, ImpactRotation);
			}
		}
	}
}