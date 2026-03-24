#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractInterface.generated.h"

// 이 클래스는 엔진 내부용이므로 건드리지 않아도 돼!
UINTERFACE(MinimalAPI)
class UInteractInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 실제로 우리가 상속받아 사용할 인터페이스 클래스
 */
class GUN_PHIRIA_API IInteractInterface
{
	GENERATED_BODY()

public:
	/** * 상호작용 실행 함수
	 * BlueprintNativeEvent: C++에서 기본 로직을 짜고, 블루프린트에서 덮어쓸 수 있게 함
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void Interact(AActor* Interactor);

	/** * 애니메이션을 재생해야 하는 물체인지 판별 (아이템 줍기: true, NPC 대화: false)
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool ShouldPlayAnimation() const;
};