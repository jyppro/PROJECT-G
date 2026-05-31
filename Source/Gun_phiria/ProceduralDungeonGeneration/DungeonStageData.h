#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "DungeonGenerator.h"
#include "DungeonStageData.generated.h"

/**
 * 스테이지(노드)별 절차적 던전 생성 데이터와 보상/UI 정보를 담는 데이터 에셋
 */
UCLASS(BlueprintType, Blueprintable)
class GUN_PHIRIA_API UDungeonStageData : public UDataAsset
{
	GENERATED_BODY()

public:
	UDungeonStageData(); // 생성자 선언

	// --------------------------------------------------
	// 1. UI 및 노드 표시 정보 (세피리아 스타일 맵 위젯용)
	// --------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage Info")
	FName StageName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage Info")
	FString StageDescription;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage Info")
	FName StageType; // 예: "Normal", "Elite", "Shop", "Boss", "Anvil"

	// --------------------------------------------------
	// 2. 던전 절차적 생성 세팅
	// --------------------------------------------------
	// [수정] 모루방, 보스방 등 단일 방 생성을 위해 ClampMin을 1로 낮췄습니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = "1", ClampMax = "50"))
	int32 NumberOfRoomsToGenerate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	TArray<FRoomPrefabData> MainRoomPrefabs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	TSubclassOf<AActor> CorridorPrefab;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	TSubclassOf<AActor> DoorPrefab;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	TSubclassOf<AActor> StageDoorPrefab;

	// --------------------------------------------------
	// 3. 등장 몬스터 및 난이도 세팅
	// --------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	TSubclassOf<class AEnemyCharacter> DefaultEnemyPrefab;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (ClampMin = "0.1"))
	float EnemyStatMultiplier;

	// --------------------------------------------------
	// 4. 보상 및 특수 방(필수 스폰) 세팅
	// --------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rewards")
	UDataTable* StageItemDataTable;

	// [추가] 던전 생성 시 무조건 등장해야 하는 콘텐츠 플래그
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rewards|Mandatory Spawns")
	bool bForceSpawnShop;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rewards|Mandatory Spawns")
	bool bForceSpawnAnvil;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rewards|Mandatory Spawns")
	bool bForceSpawnGold;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rewards|Mandatory Spawns")
	bool bForceSpawnMiniBoss;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rewards|Mandatory Spawns")
	bool bForceSpawnBoss;
};