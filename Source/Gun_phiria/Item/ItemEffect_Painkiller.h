#pragma once

#include "CoreMinimal.h"
#include "ItemEffectBase.h"
#include "ItemEffect_Painkiller.generated.h"

UCLASS(Blueprintable)
class GUN_PHIRIA_API UItemEffect_Painkiller : public UItemEffectBase
{
	GENERATED_BODY()

public:
	// 시전 시간 (초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Effect|Cast")
	float CastTime = 6.0f;

	// 총 회복할 체력
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Effect|Heal")
	float TotalHealAmount = 40.0f;

	// 회복에 걸리는 총 시간 (초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Effect|Heal")
	float HealDuration = 20.0f;

	// 이동 속도 증가량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Effect|Buff")
	float SpeedBoostAmount = 150.0f;

	// 이동 속도 증가 지속 시간 (초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Effect|Buff")
	float BoostDuration = 60.0f;

	// 함수의 매개변수 모양을 부모와 똑같이 맞춰줍니다.
	virtual bool UseItem_Implementation(AGun_phiriaCharacter* User, FName ItemID) override;
};