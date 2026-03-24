#include "BTTask_FireAtPlayer.h"
#include "AIController.h"
#include "../Enemy/EnemyCharacter.h" 

UBTTask_FireAtPlayer::UBTTask_FireAtPlayer()
{
	NodeName = TEXT("Fire At Player");
}

EBTNodeResult::Type UBTTask_FireAtPlayer::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (AIController)
	{
		AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(AIController->GetPawn());
		if (Enemy)
		{
			Enemy->FireAtPlayer();
			return EBTNodeResult::Succeeded;
		}
	}

	return EBTNodeResult::Failed;
}