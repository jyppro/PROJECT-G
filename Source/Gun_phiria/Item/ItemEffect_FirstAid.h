#pragma once

#include "CoreMinimal.h"
#include "ItemEffectBase.h"
#include "ItemEffect_FirstAid.generated.h"

UCLASS()
class GUN_PHIRIA_API UItemEffect_FirstAid : public UItemEffectBase
{
	GENERATED_BODY()

public:
	// 체력을 몇 퍼센트(%) 회복할지 에디터에서 설정 가능하게 변수화!
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Heal")
	float HealPercentage = 0.8f; // 기본값 80%

	virtual bool UseItem_Implementation(class AGun_phiriaCharacter* User) override;
};