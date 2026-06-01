#include "Gun_phiriaGameInstance.h"
#include "Gun_phiriaCharacter.h"
#include "Weapon/WeaponBase.h"

void UGun_phiriaGameInstance::GenerateRunMap()
{
	CurrentRunMap.Empty();
	CurrentNodeID = 0; // 시작 지점을 0번 노드로 고정
	int32 GlobalNodeID = 0;

	// 총 5층 구조 (0: 시작점, 1: 모루 층, 2: 일반 층, 3: 미니보스 층, 4: 보스 층)
	const int32 TotalFloors = 5;
	TArray<TArray<int32>> FloorNodeIDs;
	FloorNodeIDs.SetNum(TotalFloors);

	// 강제 등장하는 방을 제외한 순수 '일반 보상 풀'
	TArray<FName> NormalRewardTypes = { TEXT("Artifact"), TEXT("EXP"), TEXT("Enchant"), TEXT("Dice"), TEXT("Gold"), TEXT("Sapphire"), TEXT("Shop") };

	// 1. 층별 노드 생성
	for (int32 Floor = 0; Floor < TotalFloors; Floor++)
	{
		// [수정 핵심] 시작점(0층)과 보스방(4층)은 1개, 나머지 진행 층은 무조건 3개의 갈래길을 제공합니다.
		int32 NodesInThisFloor = (Floor == 0 || Floor == TotalFloors - 1) ? 1 : 3;

		// 3개의 방 중 '고정 스테이지'가 등장할 인덱스를 무작위로 하나 정합니다 (0, 1, 2 중 하나)
		int32 FixedNodeIndex = FMath::RandRange(0, NodesInThisFloor - 1);

		for (int32 i = 0; i < NodesInThisFloor; i++)
		{
			FStageNode NewNode;
			NewNode.NodeID = GlobalNodeID;
			NewNode.bIsCleared = false;
			NewNode.FloorLevel = Floor;
			NewNode.ColumnIndex = i;

			// ==========================================
			// --- [핵심] 층별 기획 확정 스폰 처리 ---
			// ==========================================
			if (Floor == 0)
			{
				// 0층의 고정석: 시작 방 (Basic Start)
				NewNode.RoomIconType = NAME_None; // 아이콘을 띄우지 않음
				// (참고: GameInstance 헤더에 BasicStartStageData가 선언되어 있어야 합니다)
				NewNode.StageData = BasicStartStageData;
			}
			else if (Floor == 1 && i == FixedNodeIndex)
			{
				// 1층의 고정석: 모루
				NewNode.RoomIconType = TEXT("Anvil");
				NewNode.StageData = AnvilStageData;
			}
			else if (Floor == TotalFloors - 2 && i == FixedNodeIndex)
			{
				// 3층(끝에서 두번째)의 고정석: 미니보스
				NewNode.RoomIconType = TEXT("MiniBoss");
				if (EliteStagePool.Num() > 0)
				{
					NewNode.StageData = EliteStagePool[FMath::RandRange(0, EliteStagePool.Num() - 1)];
				}
			}
			else if (Floor == TotalFloors - 1)
			{
				// 4층(마지막): 보스
				NewNode.RoomIconType = TEXT("Boss");
				NewNode.StageData = BossStageData;
			}
			// ==========================================
			// --- 나머지 자리: 일반 보상 랜덤 배정 ---
			// ==========================================
			else
			{
				FName SelectedReward = NormalRewardTypes[FMath::RandRange(0, NormalRewardTypes.Num() - 1)];
				NewNode.RoomIconType = SelectedReward;

				if (SelectedReward == TEXT("Shop"))
				{
					NewNode.StageData = ShopStageData;
				}
				else
				{
					if (NormalStagePool.Num() > 0)
						NewNode.StageData = NormalStagePool[FMath::RandRange(0, NormalStagePool.Num() - 1)];
				}
			}

			CurrentRunMap.Add(GlobalNodeID, NewNode);
			FloorNodeIDs[Floor].Add(GlobalNodeID);
			GlobalNodeID++;
		}
	}

	// 2. 노드 연결 (현재 층의 방에서 다음 층의 모든 방으로 점선 연결)
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