#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ItemEffectBase.generated.h"

UCLASS(Blueprintable, BlueprintType, Abstract, EditInlineNew)
class GUN_PHIRIA_API UItemEffectBase : public UObject
{
	GENERATED_BODY()

public:
	// 아이템을 사용했을 때 실행될 가상 함수 (성공적으로 사용했으면 true 반환)
	// BlueprintNativeEvent로 만들면 C++과 블루프린트 양쪽에서 효과를 자유롭게 구현 가능해!
	UFUNCTION(BlueprintNativeEvent, Category = "Item Effect")
	bool UseItem(class AGun_phiriaCharacter* User, FName ItemID);
};