#pragma once

#include "CoreMinimal.h"
#include "ItemEffectBase.h"
#include "ItemEffect_FirstAid.generated.h"

UCLASS(Blueprintable)
class GUN_PHIRIA_API UItemEffect_FirstAid : public UItemEffectBase
{
	GENERATED_BODY()

public:
	// 시전 시간 (초) - 구급상자는 보통 6초, 붕대는 4초 등으로 블루프린트에서 다르게 설정 가능
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Effect|Cast")
	float CastTime = 6.0f;

	// 체력을 몇 퍼센트(%) 회복할지 에디터에서 설정 가능하게 변수화!
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Effect|Heal")
	float HealPercentage = 0.8f; // 기본값 80%

	// [수정됨] 매개변수에 FName ItemID 추가
	virtual bool UseItem_Implementation(class AGun_phiriaCharacter* User, FName ItemID) override;
};