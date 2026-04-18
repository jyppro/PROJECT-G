#pragma once

#include "CoreMinimal.h"
#include "ThrowableProjectileBase.h"
#include "GrenadeProjectile.generated.h"

UCLASS()
class GUN_PHIRIA_API AGrenadeProjectile : public AThrowableProjectileBase
{
	GENERATED_BODY()

public:
	AGrenadeProjectile();

protected:
	virtual void Explode() override;

	// ¼ö·ùÅº Æø¹ß ÀÌÆåÆ® (³ªÀÌ¾Æ°¡¶ó)
	UPROPERTY(EditDefaultsOnly, Category = "Explosion|Effects")
	TObjectPtr<class UNiagaraSystem> ExplosionEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Explosion|Effects")
	TObjectPtr<class USoundBase> ExplosionSound;

	UPROPERTY(EditDefaultsOnly, Category = "Explosion|Damage")
	float ExplosionDamage = 150.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Explosion|Damage")
	float ExplosionRadius = 500.0f;
};