#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UBehaviorTree;

UCLASS()
class GUN_PHIRIA_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	// --- Constructor ---
	AEnemyAIController();

protected:
	// --- Lifecycle ---
	virtual void BeginPlay() override;

	// --- AI Configuration ---
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AI")
	TObjectPtr<UAIPerceptionComponent> AIPerception;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AI")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	// --- Callbacks ---
	UFUNCTION()
	void OnTargetDetected(AActor* Actor, FAIStimulus const Stimulus);

private:
	// --- Assets ---
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

};