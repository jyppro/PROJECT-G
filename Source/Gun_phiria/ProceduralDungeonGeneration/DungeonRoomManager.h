#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DungeonRoomManager.generated.h"

// --- Forward Declarations ---
class UBoxComponent;
class AEnemyCharacter;

UCLASS()
class GUN_PHIRIA_API ADungeonRoomManager : public AActor
{
	GENERATED_BODY()

public:
	// --- Constructor & API ---
	ADungeonRoomManager();
	void OnEnemyDied();

	// --- Public Data (Set by Generator) ---
	UPROPERTY()
	TArray<TObjectPtr<AActor>> ConnectedDoors;

	UPROPERTY()
	TSubclassOf<AEnemyCharacter> EnemyPrefab;

	FVector RoomSize;

	// --- Components ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> RoomTrigger;

protected:
	// --- Lifecycle ---
	virtual void BeginPlay() override;

private:
	// --- Internal Callbacks ---
	UFUNCTION()
	void OnPlayerEnter(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// --- Internal Logic ---
	void LockDoors();
	void UnlockDoors();
	void SpawnEnemies();
	bool IsSpawnLocationValid(FVector Location);

	// --- State Flags ---
	bool bIsTriggered = false;
	bool bIsCleared = false;
	int32 AliveEnemiesCount = 0;
};