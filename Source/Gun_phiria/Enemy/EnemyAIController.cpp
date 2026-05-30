#include "EnemyAIController.h"
#include "../Enemy/EnemyCharacter.h"

// Engine Headers
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "../NPC/ShopNPC.h"

AEnemyAIController::AEnemyAIController()
{
	// Perception Setup
	AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));

	if (SightConfig)
	{
		SightConfig->SightRadius = 2500.0f;
		SightConfig->LoseSightRadius = 3000.0f;
		SightConfig->PeripheralVisionAngleDegrees = 60.0f;
		SightConfig->SetMaxAge(5.0f);

		SightConfig->DetectionByAffiliation.bDetectEnemies = true;
		SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
		SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

		AIPerception->ConfigureSense(*SightConfig);
		AIPerception->SetDominantSense(SightConfig->GetSenseImplementation());
	}
}

void AEnemyAIController::BeginPlay()
{
	Super::BeginPlay();

	// Perception Event Binding
	if (AIPerception)
	{
		AIPerception->OnTargetPerceptionUpdated.AddDynamic(this, &AEnemyAIController::OnTargetDetected);
	}

	// Logic Execution
	if (BehaviorTreeAsset)
	{
		RunBehaviorTree(BehaviorTreeAsset);
	}
}

void AEnemyAIController::OnTargetDetected(AActor* Actor, FAIStimulus const Stimulus)
{
	TObjectPtr<ACharacter> PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);

	if (Actor == PlayerCharacter)
	{
		TObjectPtr<AEnemyCharacter> Enemy = Cast<AEnemyCharacter>(GetPawn());
		TObjectPtr<UBlackboardComponent> BB = GetBlackboardComponent();

		if (Stimulus.WasSuccessfullySensed())
		{
			// 시야에 플레이어가 들어왔을 때의 예외 처리
			if (Enemy)
			{
				// 1. 만약 이 적이 '상점 주인(ShopNPC)'이라면 캐스팅해 봅니다.
				if (AShopNPC* ShopNPC = Cast<AShopNPC>(Enemy))
				{
					// 2. 상점 주인인데 아직 화가 나지 않았다면(bIsHostile == false), 
					// 조준 상태로 돌입하지 않고 함수를 즉시 종료합니다!
					if (!ShopNPC->bIsHostile)
					{
						return;
					}
				}

				// 상점 주인이 아니거나, 상점 주인이 빡친 상태라면 정상적으로 전투 모드 돌입
				Enemy->bIsAiming = true;
			}

			if (BB) BB->SetValueAsObject(FName("TargetActor"), PlayerCharacter);
		}
		else
		{
			// Target Lost (시야에서 사라짐)
			if (BB) BB->ClearValue(FName("TargetActor"));
			if (Enemy) Enemy->bIsAiming = false;
		}
	}
}