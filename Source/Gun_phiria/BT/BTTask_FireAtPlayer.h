#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_FireAtPlayer.generated.h"

UCLASS()
class GUN_PHIRIA_API UBTTask_FireAtPlayer : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_FireAtPlayer();

protected:
	// 비헤이비어 트리에서 이 노드가 실행될 때 호출되는 핵심 함수입니다.
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};