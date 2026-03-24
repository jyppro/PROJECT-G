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
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};