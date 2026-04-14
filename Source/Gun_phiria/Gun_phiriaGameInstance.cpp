#include "Gun_phiriaGameInstance.h"
#include "Gun_phiriaCharacter.h"

void UGun_phiriaGameInstance::SavePlayerData(AGun_phiriaCharacter* Player, bool bKeepOnlySapphire)
{
	if (!Player || !Player->PlayerInventory) return;

	SavedSapphire = Player->CurrentSapphire;

	if (bKeepOnlySapphire)
	{
		// [마을 귀환] 사파이어 외 초기화
		SavedGold = 0;
		SavedInventory.Empty();
		SavedHelmetID = SavedVestID = SavedBackpackID = NAME_None;
		// 무기 정보도 초기화
		SavedWeapon1ID = SavedWeapon2ID = SavedPistolID = SavedThrowableID = NAME_None;
		SavedActiveSlotIndex = 0;
		SavedHealth = -1.0f;
		bHasSavedData = false;
	}
	else
	{
		// [다음 층 이동] 모든 정보 백업
		SavedGold = Player->CurrentGold;
		SavedHealth = Player->CurrentHealth;

		SavedInventory = Player->PlayerInventory->InventorySlots;
		SavedHelmetID = Player->PlayerInventory->EquippedHelmetID;
		SavedVestID = Player->PlayerInventory->EquippedVestID;
		SavedBackpackID = Player->PlayerInventory->EquippedBackpackID;

		// [추가] 전투 슬롯 정보 저장
		SavedWeapon1ID = Player->PlayerInventory->EquippedWeapon1ID;
		SavedWeapon2ID = Player->PlayerInventory->EquippedWeapon2ID;
		SavedPistolID = Player->PlayerInventory->EquippedPistolID;
		SavedThrowableID = Player->PlayerInventory->EquippedThrowableID;

		SavedActiveSlotIndex = Player->ActiveWeaponSlot;

		SavedCurrentWeight = Player->PlayerInventory->CurrentWeight;
		SavedMaxWeight = Player->PlayerInventory->MaxWeight;

		bHasSavedData = true;
	}
}

void UGun_phiriaGameInstance::LoadPlayerData(AGun_phiriaCharacter* Player)
{
	if (!Player || !Player->PlayerInventory) return;

	Player->CurrentSapphire = SavedSapphire;

	if (bHasSavedData)
	{
		Player->CurrentGold = SavedGold;

		if (SavedHealth > 0.0f)
		{
			Player->CurrentHealth = SavedHealth;
		}

		Player->PlayerInventory->InventorySlots = SavedInventory;
		Player->PlayerInventory->EquippedHelmetID = SavedHelmetID;
		Player->PlayerInventory->EquippedVestID = SavedVestID;
		Player->PlayerInventory->EquippedBackpackID = SavedBackpackID;

		// [추가] 전투 슬롯 정보 복구
		Player->PlayerInventory->EquippedWeapon1ID = SavedWeapon1ID;
		Player->PlayerInventory->EquippedWeapon2ID = SavedWeapon2ID;
		Player->PlayerInventory->EquippedPistolID = SavedPistolID;
		Player->PlayerInventory->EquippedThrowableID = SavedThrowableID;

		Player->ActiveWeaponSlot = SavedActiveSlotIndex;

		Player->PlayerInventory->CurrentWeight = SavedCurrentWeight;
		Player->PlayerInventory->MaxWeight = SavedMaxWeight;
	}
}