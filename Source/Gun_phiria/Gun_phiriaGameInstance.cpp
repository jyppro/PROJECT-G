#include "Gun_phiriaGameInstance.h"
#include "Gun_phiriaCharacter.h"

void UGun_phiriaGameInstance::SavePlayerData(AGun_phiriaCharacter* Player, bool bKeepOnlySapphire)
{
	if (!Player || !Player->PlayerInventory) return;

	// 사파이어는 언제나 영구 보존!
	SavedSapphire = Player->CurrentSapphire;

	if (bKeepOnlySapphire)
	{
		// [마을 귀환] 사파이어 빼고 다 날립니다.
		SavedGold = 0;
		SavedInventory.Empty();
		SavedHelmetID = NAME_None;
		SavedVestID = NAME_None;
		SavedBackpackID = NAME_None;
		SavedHealth = -1.0f; // 초기화
		bHasSavedData = false; // 복구할 일반 데이터가 없음을 의미
	}
	else
	{
		// [다음 층 이동] 모든 정보를 안전하게 백업합니다.
		SavedGold = Player->CurrentGold;
		SavedHealth = Player->CurrentHealth;

		SavedInventory = Player->PlayerInventory->InventorySlots;
		SavedHelmetID = Player->PlayerInventory->EquippedHelmetID;
		SavedVestID = Player->PlayerInventory->EquippedVestID;
		SavedBackpackID = Player->PlayerInventory->EquippedBackpackID;
		SavedCurrentWeight = Player->PlayerInventory->CurrentWeight;
		SavedMaxWeight = Player->PlayerInventory->MaxWeight;

		bHasSavedData = true;
	}
}

void UGun_phiriaGameInstance::LoadPlayerData(AGun_phiriaCharacter* Player)
{
	if (!Player || !Player->PlayerInventory) return;

	// 사파이어는 마을이든 던전이든 항상 적용!
	Player->CurrentSapphire = SavedSapphire;

	// 복구할 데이터가 있다면 (다음 층으로 넘어온 경우)
	if (bHasSavedData)
	{
		Player->CurrentGold = SavedGold;

		if (SavedHealth > 0.0f)
		{
			Player->CurrentHealth = SavedHealth;
		}

		// 인벤토리 복구
		Player->PlayerInventory->InventorySlots = SavedInventory;
		Player->PlayerInventory->EquippedHelmetID = SavedHelmetID;
		Player->PlayerInventory->EquippedVestID = SavedVestID;
		Player->PlayerInventory->EquippedBackpackID = SavedBackpackID;
		Player->PlayerInventory->CurrentWeight = SavedCurrentWeight;
		Player->PlayerInventory->MaxWeight = SavedMaxWeight;

		// (선택) 장착된 아이템의 3D 메쉬를 다시 띄워주려면 데이터 테이블을 읽어서 UpdateEquipmentVisuals 등을 호출해야 할 수 있음.
	}
}