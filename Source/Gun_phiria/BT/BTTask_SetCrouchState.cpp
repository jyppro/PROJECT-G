#include "BTTask_SetCrouchState.h"
#include "AIController.h"
#include "../Enemy/EnemyCharacter.h"

UBTTask_SetCrouchState::UBTTask_SetCrouchState()
{
	NodeName = TEXT("Set Crouch State");
	bShouldCrouch = true; // 기본값은 숙이는 걸로 설정
	CrouchProbability = 100.0f; // 초기값 100%
}

EBTNodeResult::Type UBTTask_SetCrouchState::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (AIController)
	{
		AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(AIController->GetPawn());
		if (Enemy)
		{
			// bShouldCrouch가 켜져 있다면 확률 계산 시작!
			if (bShouldCrouch)
			{
				// ★ 0~100 사이의 랜덤 숫자를 뽑아서 지정한 확률보다 낮으면 숙임!
				if (FMath::FRandRange(0.0f, 100.0f) <= CrouchProbability)
				{
					Enemy->Crouch();
					Enemy->bIsCrouching = true;
				}
				else
				{
					// 확률에 실패하면 일어섬
					Enemy->UnCrouch();
					Enemy->bIsCrouching = false;
				}
			}
			else
			{
				// bShouldCrouch가 꺼져있으면 무조건 일어섬
				Enemy->UnCrouch();
				Enemy->bIsCrouching = false;
			}

			return EBTNodeResult::Succeeded;
		}
	}
	return EBTNodeResult::Failed;
}