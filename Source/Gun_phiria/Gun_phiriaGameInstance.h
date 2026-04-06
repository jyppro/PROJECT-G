#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "component/InventoryComponent.h" // 인벤토리 구조체(FInventorySlot)를 사용하기 위해 포함
#include "Gun_phiriaGameInstance.generated.h"

UCLASS()
class GUN_PHIRIA_API UGun_phiriaGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	// --- 보관함(변수들) ---
	UPROPERTY(BlueprintReadWrite, Category = "SaveData")
	int32 SavedGold = 0;

	UPROPERTY(BlueprintReadWrite, Category = "SaveData")
	int32 SavedSapphire = 0;

	// 인벤토리 슬롯과 장착 정보 백업
	UPROPERTY()
	TArray<FInventorySlot> SavedInventory;

	FName SavedHelmetID;
	FName SavedVestID;
	FName SavedBackpackID;
	float SavedCurrentWeight;
	float SavedMaxWeight;

	// 체력 백업 (선택 사항: 던전에서 깎인 체력을 다음 층에 유지하고 싶을 때)
	float SavedHealth = -1.0f;

	// 저장된 데이터가 있는지 여부
	bool bHasSavedData = false;

	// --- 함수들 ---
	// 문을 지날 때 호출할 저장 함수
	void SavePlayerData(class AGun_phiriaCharacter* Player, bool bKeepOnlySapphire);

	// 맵이 시작될 때 호출할 불러오기 함수
	void LoadPlayerData(class AGun_phiriaCharacter* Player);
};