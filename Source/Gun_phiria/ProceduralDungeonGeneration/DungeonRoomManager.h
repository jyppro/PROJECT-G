#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DungeonRoomManager.generated.h"

UCLASS()
class GUN_PHIRIA_API ADungeonRoomManager : public AActor
{
	GENERATED_BODY()

public:
	ADungeonRoomManager();

protected:
	virtual void BeginPlay() override;

public:
	// 플레이어 입장을 감지할 투명 박스
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UBoxComponent* RoomTrigger;

	// 던전 생성기에서 전달받을 데이터들
	UPROPERTY()
	TArray<AActor*> ConnectedDoors;

	UPROPERTY()
	TSubclassOf<class AEnemyCharacter> EnemyPrefab;

	FVector RoomSize;

	// 적이 죽었을 때 보고받을 함수
	void OnEnemyDied();

private:
	bool bIsTriggered = false;
	bool bIsCleared = false;
	int32 AliveEnemiesCount = 0;

	UFUNCTION()
	void OnPlayerEnter(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void LockDoors();
	void UnlockDoors();
	void SpawnEnemies();
	bool IsSpawnLocationValid(FVector Location);
};