#include "EnemyAIController.h"
#include "../Enemy/EnemyCharacter.h"

// Engine Headers
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"

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
			// Target Acquired
			if (BB) BB->SetValueAsObject(FName("TargetActor"), PlayerCharacter);
			if (Enemy) Enemy->bIsAiming = true;
		}
		else
		{
			// Target Lost
			if (BB) BB->ClearValue(FName("TargetActor"));
			if (Enemy) Enemy->bIsAiming = false;
		}
	}
}