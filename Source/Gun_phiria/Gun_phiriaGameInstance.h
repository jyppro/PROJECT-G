#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "component/InventoryComponent.h"
#include "ProceduralDungeonGeneration/DungeonStageData.h" 
#include "Gun_phiriaGameInstance.generated.h"

// 맵 노드 구조체 선언
USTRUCT(BlueprintType)
struct FStageNode
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 NodeID = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName RoomIconType = TEXT("Normal");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UDungeonStageData* StageData = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<int32> ConnectedNextNodes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsCleared = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 FloorLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ColumnIndex = 0;
};

UCLASS()
class GUN_PHIRIA_API UGun_phiriaGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	// =================================================
	// [기존] 플레이어 세이브 데이터 보존용 (생략 없이 유지)
	// =================================================
	UPROPERTY(BlueprintReadWrite, Category = "SaveData")
	int32 SavedGold = 0;

	UPROPERTY(BlueprintReadWrite, Category = "SaveData")
	int32 SavedSapphire = 0;

	UPROPERTY()
	TArray<FInventorySlot> SavedInventory;

	FName SavedHelmetID;
	float SavedHelmetDurability;

	FName SavedVestID;
	float SavedVestDurability;

	FName SavedBackpackID;

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
	UPROPERTY(BlueprintReadWrite, Category = "Run Data")
	TMap<int32, FStageNode> CurrentRunMap;

	UPROPERTY(BlueprintReadWrite, Category = "Run Data")
	int32 CurrentNodeID = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Run Data")
	UDungeonStageData* NextStageData = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Run Logic")
	void GenerateRunMap();

	// =================================================
	// [수정] 맵 생성용 스테이지 데이터 풀 (완벽 통합 버전)
	// =================================================

	// 1. 고정 등장 풀 (Fixed)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run Data|Pools|Fixed")
	UDungeonStageData* BasicStartStageData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run Data|Pools|Fixed")
	UDungeonStageData* AnvilStageData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run Data|Pools|Fixed")
	TArray<UDungeonStageData*> MiniBossStagePool;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run Data|Pools|Fixed")
	TArray<UDungeonStageData*> BossStagePool;

	// 2. 무작위 등장 일반 스테이지 풀 (Normal)
	// 상점, 골드, 사파이어, 아티팩트, 경험치, 주사위, 인챈트 등을 모두 이 배열 하나에 몰아넣습니다!
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run Data|Pools|Normal")
	TArray<UDungeonStageData*> NormalStagePool;
};