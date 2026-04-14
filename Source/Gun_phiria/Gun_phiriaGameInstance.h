#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "component/InventoryComponent.h"
#include "Gun_phiriaGameInstance.generated.h"

UCLASS()
class GUN_PHIRIA_API UGun_phiriaGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "SaveData")
	int32 SavedGold = 0;

	UPROPERTY(BlueprintReadWrite, Category = "SaveData")
	int32 SavedSapphire = 0;

	UPROPERTY()
	TArray<FInventorySlot> SavedInventory;

	// --- 장비 정보 ---
	FName SavedHelmetID;
	FName SavedVestID;
	FName SavedBackpackID;

	// [추가] 전투 슬롯(무기) 정보 백업
	FName SavedWeapon1ID;
	FName SavedWeapon2ID;
	FName SavedPistolID;
	FName SavedThrowableID;

	UPROPERTY(BlueprintReadWrite, Category = "SaveData")
	int32 SavedActiveSlotIndex = 0;

	float SavedCurrentWeight;
	float SavedMaxWeight;
	float SavedHealth = -1.0f;
	bool bHasSavedData = false;

	void SavePlayerData(class AGun_phiriaCharacter* Player, bool bKeepOnlySapphire);
	void LoadPlayerData(class AGun_phiriaCharacter* Player);
};