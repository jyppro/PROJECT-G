#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractInterface.generated.h"

UINTERFACE(MinimalAPI)
class UInteractInterface : public UInterface
{
	GENERATED_BODY()
};

class GUN_PHIRIA_API IInteractInterface
{
	GENERATED_BODY()

public:
	// 상호작용 실행 함수
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void Interact(AActor* Interactor);

	// 애니메이션을 재생해야 하는 물체인지 판별 (아이템 줍기: true, NPC 대화: false)
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool ShouldPlayAnimation() const;
};