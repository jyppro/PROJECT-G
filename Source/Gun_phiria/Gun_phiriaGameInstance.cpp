#include "Gun_phiriaGameInstance.h"
#include "Gun_phiriaCharacter.h"
#include "Weapon/WeaponBase.h"

void UGun_phiriaGameInstance::GenerateRunMap()
{
	CurrentRunMap.Empty();
	// [UI 버그 해결] 시작 지점을 명확히 0번 노드로 고정! 
	// 이 값이 0이어야 UI에서 1층(0번 방)에 플레이어 아이콘과 선이 나옵니다.
	CurrentNodeID = 0;

	int32 GlobalNodeID = 0;

	// 총 5층 구조 (0: 시작점, 1: 모루 층, 2: 일반 층, 3: 미니보스 층, 4: 보스 층)
	const int32 TotalFloors = 5;
	TArray<TArray<int32>> FloorNodeIDs;
	FloorNodeIDs.SetNum(TotalFloors);

	// 1. 층별 노드 생성
	for (int32 Floor = 0; Floor < TotalFloors; Floor++)
	{
		int32 NodesInThisFloor = (Floor == 0 || Floor == TotalFloors - 1) ? 1 : 3;
		int32 FixedNodeIndex = FMath::RandRange(0, NodesInThisFloor - 1);

		for (int32 i = 0; i < NodesInThisFloor; i++)
		{
			FStageNode NewNode;
			NewNode.NodeID = GlobalNodeID;
			NewNode.bIsCleared = false;
			NewNode.FloorLevel = Floor;
			NewNode.ColumnIndex = i;

			// ==========================================
			// 1. 고정(Fixed) 스테이지 스폰
			// ==========================================
			if (Floor == 0)
			{
				NewNode.RoomIconType = NAME_None;
				NewNode.StageData = BasicStartStageData;
			}
			else if (Floor == 1 && i == FixedNodeIndex)
			{
				NewNode.RoomIconType = TEXT("Anvil");
				NewNode.StageData = AnvilStageData;
			}
			else if (Floor == TotalFloors - 2 && i == FixedNodeIndex)
			{
				NewNode.RoomIconType = TEXT("MiniBoss");
				if (MiniBossStagePool.Num() > 0)
					NewNode.StageData = MiniBossStagePool[FMath::RandRange(0, MiniBossStagePool.Num() - 1)];
			}
			else if (Floor == TotalFloors - 1)
			{
				NewNode.RoomIconType = TEXT("Boss");
				if (BossStagePool.Num() > 0)
					NewNode.StageData = BossStagePool[FMath::RandRange(0, BossStagePool.Num() - 1)];
			}
			// ==========================================
			// 2. 묶여있는 일반(Normal) 풀에서 무작위 스폰
			// ==========================================
			else
			{
				if (NormalStagePool.Num() > 0)
				{
					// 배열에서 무작위 데이터 에셋 하나를 뽑습니다.
					UDungeonStageData* SelectedStage = NormalStagePool[FMath::RandRange(0, NormalStagePool.Num() - 1)];
					NewNode.StageData = SelectedStage;

					// [핵심] 뽑은 에셋 안에 적혀있는 'StageType'을 UI 아이콘 이름으로 그대로 씁니다!
					if (SelectedStage)
					{
						NewNode.RoomIconType = SelectedStage->StageType;
					}
				}
			}

			CurrentRunMap.Add(GlobalNodeID, NewNode);
			FloorNodeIDs[Floor].Add(GlobalNodeID);
			GlobalNodeID++;
		}
	}

	// 2. 노드 연결 (선 긋기 용도)
	for (int32 Floor = 0; Floor < TotalFloors - 1; Floor++)
	{
		for (int32 CurrNode : FloorNodeIDs[Floor])
		{
			for (int32 NextNode : FloorNodeIDs[Floor + 1])
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