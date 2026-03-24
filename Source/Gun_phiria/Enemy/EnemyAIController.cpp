#include "EnemyAIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "../Enemy/EnemyCharacter.h"

AEnemyAIController::AEnemyAIController()
{
	// 퍼셉션(시야) 컴포넌트 생성
	/*AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));*/

	PerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));

	// 시야 설정 (시야 반경, 잃어버리는 반경, 시야각 등)
	SightConfig->SightRadius = 2500.0f;					// 발견 반경: 25미터
	SightConfig->LoseSightRadius = 3000.0f;				// 잃어버리는 반경: 30미터
	SightConfig->PeripheralVisionAngleDegrees = 60.0f;	// 시야각: 60도
	SightConfig->SetMaxAge(5.0f);						// 기억 시간: 5초

	// 감지 대상 설정 (일단 모든 것을 감지하게 세팅)
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

	/*AIPerception->ConfigureSense(*SightConfig);
	AIPerception->SetDominantSense(SightConfig->GetSenseImplementation());*/

	PerceptionComponent->ConfigureSense(*SightConfig);
	PerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
}

void AEnemyAIController::BeginPlay()
{
	Super::BeginPlay();

	// 시야에 무언가 들어오거나 나갈 때 OnTargetDetected 함수 실행 연결
	//AIPerception->OnTargetPerceptionUpdated.AddDynamic(this, &AEnemyAIController::OnTargetDetected);

	//// 비헤이비어 트리 실행
	//if (BehaviorTreeAsset)
	//{
	//	RunBehaviorTree(BehaviorTreeAsset);
	//}

	if (PerceptionComponent)
	{
		PerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AEnemyAIController::OnTargetDetected);
	}

	if (BehaviorTreeAsset)
	{
		RunBehaviorTree(BehaviorTreeAsset);
	}
}

void AEnemyAIController::OnTargetDetected(AActor* Actor, FAIStimulus const Stimulus)
{
	ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);

	if (Actor == PlayerCharacter)
	{
		// ★ [새로 추가] 내가 조종하고 있는 적 캐릭터(Pawn)를 가져옵니다.
		AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(GetPawn());

		if (Stimulus.WasSuccessfullySensed())
		{
			// 플레이어를 발견함!
			GetBlackboardComponent()->SetValueAsObject(FName("TargetActor"), PlayerCharacter);

			// ★ [새로 추가] 무기를 들어 올려 조준 자세를 취하게 만듭니다!
			if (Enemy) Enemy->bIsAiming = true;
		}
		else
		{
			// 플레이어를 놓침!
			GetBlackboardComponent()->ClearValue(FName("TargetActor"));

			// ★ [새로 추가] 무기를 내리고 대기 자세로 돌아갑니다.
			if (Enemy) Enemy->bIsAiming = false;
		}
	}
}