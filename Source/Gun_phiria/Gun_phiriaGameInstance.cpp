#include "Gun_phiriaGameInstance.h"
#include "Gun_phiriaCharacter.h"
#include "Weapon/WeaponBase.h"

void UGun_phiriaGameInstance::GenerateRunMap()
{
	CurrentRunMap.Empty();
	CurrentNodeID = 0;
	int32 GlobalNodeID = 0;

	// 총 6층 (0: 모루, 1~2: 일반파밍, 3: 미니보스, 4: 일반파밍, 5: 보스)
	const int32 TotalFloors = 6;
	TArray<TArray<int32>> FloorNodeIDs;
	FloorNodeIDs.SetNum(TotalFloors);

	// [수정됨] 현재 구현된 일반 방은 골드와 상점뿐이므로 두 가지만 풀에 넣습니다.
	TArray<FName> RewardTypes = { TEXT("Gold"), TEXT("Shop") };

	// 1. 층별 노드 생성
	for (int32 Floor = 0; Floor < TotalFloors; Floor++)
	{
		int32 NodesInThisFloor = (Floor == 0 || Floor == TotalFloors - 1) ? 1 : 3;

		for (int32 i = 0; i < NodesInThisFloor; i++)
		{
			FStageNode NewNode;
			NewNode.NodeID = GlobalNodeID;
			NewNode.bIsCleared = false;
			NewNode.FloorLevel = Floor;
			NewNode.ColumnIndex = i;

			// 층별 고정 노드 할당 및 실제 StageData 연결
			if (Floor == 0)
			{
				NewNode.RoomIconType = TEXT("Anvil");
				NewNode.StageData = AnvilStageData; // [수정됨] 모루 데이터 할당
			}
			else if (Floor == TotalFloors - 1)
			{
				NewNode.RoomIconType = TEXT("Boss");
				NewNode.StageData = BossStageData;  // [수정됨] 보스 데이터 할당
			}
			else if (Floor == 3)
			{
				NewNode.RoomIconType = TEXT("MiniBoss");
				// [수정됨] 미니보스 풀에서 무작위 하나 선택
				if (EliteStagePool.Num() > 0)
				{
					NewNode.StageData = EliteStagePool[FMath::RandRange(0, EliteStagePool.Num() - 1)];
				}
			}
			else
			{
				// 나머지 일반 층은 골드(NormalPool)와 상점(ShopData) 중 무작위 배정
				FName SelectedReward = RewardTypes[FMath::RandRange(0, RewardTypes.Num() - 1)];
				NewNode.RoomIconType = SelectedReward;

				if (SelectedReward == TEXT("Gold"))
				{
					if (NormalStagePool.Num() > 0)
					{
						NewNode.StageData = NormalStagePool[FMath::RandRange(0, NormalStagePool.Num() - 1)];
					}
				}
				else if (SelectedReward == TEXT("Shop"))
				{
					NewNode.StageData = ShopStageData;
				}
			}

			CurrentRunMap.Add(GlobalNodeID, NewNode);
			FloorNodeIDs[Floor].Add(GlobalNodeID);
			GlobalNodeID++;
		}
	}

	// 2. 노드 연결 (기존 코드와 동일하게 무조건 3갈래 연결)
	for (int32 Floor = 0; Floor < TotalFloors - 1; Floor++)
	{
		TArray<int32>& CurrentLayer = FloorNodeIDs[Floor];
		TArray<int32>& NextLayer = FloorNodeIDs[Floor + 1];

		for (int32 CurrNode : CurrentLayer)
		{
			for (int32 NextNode : NextLayer)
			{
				CurrentRunMap[CurrNode].ConnectedNextNodes.Add(NextNode);
			}
		}
	}
}

// [기존 코드 유지]
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
		SavedHelmetDurability = 0.0f;
		SavedVestDurability = 0.0f;

		// 무기 정보도 초기화
		SavedWeapon1ID = SavedWeapon2ID = SavedPistolID = SavedThrowableID = NAME_None;
		SavedWeapon1Ammo = SavedWeapon2Ammo = SavedThrowableAmmo = 0;
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
		SavedHelmetDurability = Player->PlayerInventory->CurrentHelmetDurability;

		SavedVestID = Player->PlayerInventory->EquippedVestID;
		SavedVestDurability = Player->PlayerInventory->CurrentVestDurability;

		SavedBackpackID = Player->PlayerInventory->EquippedBackpackID;

		// 전투 슬롯 정보 저장
		SavedWeapon1ID = Player->PlayerInventory->EquippedWeapon1ID;
		SavedWeapon2ID = Player->PlayerInventory->EquippedWeapon2ID;
		SavedPistolID = Player->PlayerInventory->EquippedPistolID;
		SavedThrowableID = Player->PlayerInventory->EquippedThrowableID;

		SavedActiveSlotIndex = Player->ActiveWeaponSlot;

		SavedCurrentWeight = Player->PlayerInventory->CurrentWeight;
		SavedMaxWeight = Player->PlayerInventory->MaxWeight;

		if (Player->WeaponSlots.IsValidIndex(1) && Player->WeaponSlots[1])
		{
			SavedWeapon1Ammo = Player->WeaponSlots[1]->CurrentAmmo;
		}

		if (Player->WeaponSlots.IsValidIndex(2) && Player->WeaponSlots[2])
		{
			SavedWeapon2Ammo = Player->WeaponSlots[2]->CurrentAmmo;
		}

		bHasSavedData = true;
	}
}

// [기존 코드 유지]
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
		Player->PlayerInventory->CurrentHelmetDurability = SavedHelmetDurability;

		Player->PlayerInventory->EquippedVestID = SavedVestID;
		Player->PlayerInventory->CurrentVestDurability = SavedVestDurability;

		Player->PlayerInventory->EquippedBackpackID = SavedBackpackID;

		// 전투 슬롯 정보 복구
		Player->PlayerInventory->EquippedWeapon1ID = SavedWeapon1ID;
		Player->PlayerInventory->EquippedWeapon2ID = SavedWeapon2ID;
		Player->PlayerInventory->EquippedPistolID = SavedPistolID;
		Player->PlayerInventory->EquippedThrowableID = SavedThrowableID;

		Player->ActiveWeaponSlot = SavedActiveSlotIndex;

		Player->PlayerInventory->CurrentWeight = SavedCurrentWeight;
		Player->PlayerInventory->MaxWeight = SavedMaxWeight;
	}
}