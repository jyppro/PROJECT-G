#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_SetCrouchState.generated.h"

UCLASS()
class GUN_PHIRIA_API UBTTask_SetCrouchState : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_SetCrouchState();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

public:
	// 블루프린트에서 체크박스로 "숙일지(True) 일어설지(False)" 결정할 변수
	UPROPERTY(EditAnywhere, Category = "Node")
	bool bShouldCrouch;

	// ★ [새로 추가] 몇 퍼센트의 확률로 숙일지 결정 (기본값 100%)
	UPROPERTY(EditAnywhere, Category = "Node", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float CrouchProbability;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	struct FBlackboardKeySelector HasCrouchedKey;
};