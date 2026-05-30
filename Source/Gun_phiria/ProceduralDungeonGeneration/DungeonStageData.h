#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "DungeonStageData.generated.h"

// [주의] ADungeonGenerator.h에 이미 정의되어 있다면, 기존 것을 사용하고 이 선언은 지워줘!
USTRUCT(BlueprintType)
struct FMainRoomPrefab
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room")
	TSubclassOf<AActor> RoomClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room")
	FVector2D Size;

	// 등장 가중치
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room")
	float SpawnWeight;
};

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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = "3", ClampMax = "50"))
	int32 NumberOfRoomsToGenerate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	TArray<FMainRoomPrefab> MainRoomPrefabs;

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
	// 4. 보상 및 특수 방 세팅
	// --------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rewards")
	UDataTable* StageItemDataTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rewards")
	bool bForceSpawnShop;
};