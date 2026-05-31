#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "component/InventoryComponent.h"
#include "ProceduralDungeonGeneration/DungeonStageData.h" // [추가] 스테이지 데이터 에셋 헤더 포함
#include "Gun_phiriaGameInstance.generated.h"

// [추가] 맵 노드 구조체 선언
USTRUCT(BlueprintType)
struct FStageNode
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 NodeID = -1;

	// UI에 띄울 아이콘 타입을 결정하는 태그 (예: Normal, Boss, Shop 등)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName RoomIconType = TEXT("Normal");

	// 이 노드에 배정된 실제 던전 생성 데이터
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UDungeonStageData* StageData = nullptr;

	// 다음으로 이동 가능한 노드들의 ID 목록
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<int32> ConnectedNextNodes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsCleared = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 FloorLevel = 0; // 이 노드가 몇 층에 있는지 (0층 ~ 5층)

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ColumnIndex = 0; // 이 노드가 층 내에서 몇 번째 열에 있는지 (0: 좌, 1: 중앙, 2: 우)
};

UCLASS()
class GUN_PHIRIA_API UGun_phiriaGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	// =================================================
	// [기존] 플레이어 세이브 데이터 보존용
	// =================================================
	UPROPERTY(BlueprintReadWrite, Category = "SaveData")
	int32 SavedGold = 0;

	UPROPERTY(BlueprintReadWrite, Category = "SaveData")
	int32 SavedSapphire = 0;

	UPROPERTY()
	TArray<FInventorySlot> SavedInventory;

	// --- 장비 정보 ---
	FName SavedHelmetID;
	float SavedHelmetDurability;

	FName SavedVestID;
	float SavedVestDurability;

	FName SavedBackpackID;

	// 전투 슬롯(무기) 정보 백업
	FName SavedWeapon1ID;
	FName SavedWeapon2ID;
	FName SavedPistolID;
	FName SavedThrowableID;

	int32 SavedWeapon1Ammo = 0;
	int32 SavedWeapon2Ammo = 0;
	int32 SavedThrowableAmmo = 0;

	UPROPERTY(BlueprintReadWrite, Category = "SaveData")
	int32 SavedActiveSlotIndex = 0;

	float SavedCurrentWeight;
	float SavedMaxWeight;
	float SavedHealth = -1.0f;
	bool bHasSavedData = false;

	void SavePlayerData(class AGun_phiriaCharacter* Player, bool bKeepOnlySapphire);
	void LoadPlayerData(class AGun_phiriaCharacter* Player);

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Game Data|Shop")
	bool bIsShopKeeperKilled = false;


	// =================================================
	// [추가] 런(Run) 기반 맵 진행 데이터
	// =================================================
	// 전체 노드 맵 (Key: NodeID, Value: NodeData)
	UPROPERTY(BlueprintReadWrite, Category = "Run Data")
	TMap<int32, FStageNode> CurrentRunMap;

	// 플레이어가 현재 위치한 노드의 ID
	UPROPERTY(BlueprintReadWrite, Category = "Run Data")
	int32 CurrentNodeID = 0;

	// 다음에 생성할 던전의 데이터 (DungeonGenerator가 읽어갈 데이터)
	UPROPERTY(BlueprintReadWrite, Category = "Run Data")
	UDungeonStageData* NextStageData = nullptr;

	// 런 맵을 절차적으로 생성하는 함수
	UFUNCTION(BlueprintCallable, Category = "Run Logic")
	void GenerateRunMap();

	// =================================================
	// [추가] 맵 생성용 스테이지 데이터 풀 (블루프린트에서 할당)
	// =================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run Data|Pools")
	UDungeonStageData* BasicStartStageData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run Data|Pools")
	TArray<UDungeonStageData*> NormalStagePool; // 일반 방 후보군

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run Data|Pools")
	TArray<UDungeonStageData*> EliteStagePool;  // 엘리트/미니보스 방 후보군

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run Data|Pools")
	UDungeonStageData* BossStageData;           // 최종 보스 방 데이터

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run Data|Pools")
	UDungeonStageData* ShopStageData;           // 상점 방 고정 데이터 (세피리아식)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run Data|Pools")
	UDungeonStageData* AnvilStageData;
};