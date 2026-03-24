#include "BTTask_SetCrouchState.h"
#include "AIController.h"
#include "../Enemy/EnemyCharacter.h"
#include "BehaviorTree/BlackboardComponent.h"

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
			if (bShouldCrouch)
			{
				if (FMath::FRandRange(0.0f, 100.0f) <= CrouchProbability)
				{
					// 1. 적을 숙이게 만듦
					Enemy->Crouch();
					Enemy->bIsCrouching = true;

					// ★ [추가] 2. 블랙보드 컴포넌트를 가져와서 '한 번이라도 숙였음'을 True로 기록!
					UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
					if (BlackboardComp)
					{
						// 에디터에서 선택한 키 이름에 접근해 Bool 값을 강제로 True로 만듭니다.
						BlackboardComp->SetValueAsBool(HasCrouchedKey.SelectedKeyName, true);
					}
				}
				else
				{
					Enemy->UnCrouch();
					Enemy->bIsCrouching = false;
				}
			}
			else
			{
				Enemy->UnCrouch();
				Enemy->bIsCrouching = false;
			}

			return EBTNodeResult::Succeeded;
		}
	}
	return EBTNodeResult::Failed;
}