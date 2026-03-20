#include "BTTask_FireAtPlayer.h"
#include "AIController.h"
#include "../Enemy/EnemyCharacter.h" 

UBTTask_FireAtPlayer::UBTTask_FireAtPlayer()
{
	// 비헤이비어 트리 에디터에서 보여질 노드의 이름을 설정합니다.
	NodeName = TEXT("Fire At Player");
}

EBTNodeResult::Type UBTTask_FireAtPlayer::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// 1. 이 트리를 실행 중인 AI 컨트롤러를 가져옵니다.
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (AIController)
	{
		// 2. 컨트롤러가 조종 중인 폰(Pawn)을 적 캐릭터로 형변환합니다.
		AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(AIController->GetPawn());
		if (Enemy)
		{
			// 3. 우리가 만들어둔 사격 함수를 실행합니다!
			Enemy->FireAtPlayer();

			// 4. "행동 성공!" 이라고 트리에 보고합니다.
			return EBTNodeResult::Succeeded;
		}
	}

	return EBTNodeResult::Failed; // 실패 시
}