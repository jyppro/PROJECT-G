#include "InventoryComponent.h"
#include "Engine/DataTable.h"
#include "../Item/ItemEffectBase.h"
#include "../Gun_phiriaCharacter.h"
#include "Blueprint/UserWidget.h" // ADD THIS LINE
#include "../UI/InventoryMainWidget.h"
#include "../Weapon/WeaponBase.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// 게임이 시작될 때, 지정된 칸 수(MaxSlots)만큼 빈 슬롯을 미리 만들어 둡니다.
	InventorySlots.SetNum(MaxSlots);
}

int32 UInventoryComponent::AddItem(FName ItemID, int32 Quantity)
{
	if (Quantity <= 0 || ItemID.IsNone() || !ItemDataTable) return Quantity;

	FItemData* ItemInfo = ItemDataTable->FindRow<FItemData>(ItemID, TEXT("AddItem"));
	if (!ItemInfo) return Quantity;

	int32 RemainingToAdd = Quantity;

	// 1. 기존 슬롯에 겹치기
	if (ItemInfo->MaxStackSize > 1)
	{
		for (int32 i = 0; i < InventorySlots.Num(); i++)
		{
			if (!InventorySlots[i].IsEmpty() && InventorySlots[i].ItemID == ItemID)
			{
				int32 SpaceLeft = ItemInfo->MaxStackSize - InventorySlots[i].Quantity;
				if (SpaceLeft > 0)
				{
					// 현재 남은 무게로 몇 개나 더 넣을 수 있는지 실시간 체크
					float AvailableWeight = MaxWeight - CurrentWeight;
					int32 WeightPossible = (ItemInfo->ItemWeight > 0.f) ? FMath::FloorToInt(AvailableWeight / ItemInfo->ItemWeight) : RemainingToAdd;

					int32 AmountToAdd = FMath::Min(FMath::Min(RemainingToAdd, SpaceLeft), WeightPossible);

					if (AmountToAdd > 0)
					{
						InventorySlots[i].Quantity += AmountToAdd;
						CurrentWeight += (ItemInfo->ItemWeight * AmountToAdd);
						RemainingToAdd -= AmountToAdd;
					}
					if (RemainingToAdd <= 0 || WeightPossible <= 0) return RemainingToAdd;
				}
			}
		}
	}

	// 2. 새 슬롯에 넣기
	for (int32 i = 0; i < InventorySlots.Num(); i++)
	{
		if (InventorySlots[i].IsEmpty())
		{
			float AvailableWeight = MaxWeight - CurrentWeight;
			int32 WeightPossible = (ItemInfo->ItemWeight > 0.f) ? FMath::FloorToInt(AvailableWeight / ItemInfo->ItemWeight) : RemainingToAdd;

			int32 AmountToAdd = FMath::Min(FMath::Min(RemainingToAdd, ItemInfo->MaxStackSize), WeightPossible);

			if (AmountToAdd > 0)
			{
				InventorySlots[i].ItemID = ItemID;
				InventorySlots[i].Quantity = AmountToAdd;
				InventorySlots[i].CurrentDurability = ItemInfo->MaxDurability;

				CurrentWeight += (ItemInfo->ItemWeight * AmountToAdd);
				RemainingToAdd -= AmountToAdd;
			}
			if (RemainingToAdd <= 0 || WeightPossible <= 0) return RemainingToAdd;
		}
	}

	return RemainingToAdd;
}

bool UInventoryComponent::RemoveItem(FName ItemID, int32 Quantity)
{
	if (Quantity <= 0 || ItemID.IsNone() || !ItemDataTable) return false;

	// 데이터 테이블에서 정보 찾기 (버릴 때 무게를 빼주기 위함)
	FItemData* ItemInfo = ItemDataTable->FindRow<FItemData>(ItemID, TEXT("RemoveItem"));

	for (int32 i = 0; i < InventorySlots.Num(); i++)
	{
		if (InventorySlots[i].ItemID == ItemID)
		{
			// 지우려는 개수와 실제 슬롯에 있는 개수 중 작은 값을 뺌
			int32 RemovedAmount = FMath::Min(Quantity, InventorySlots[i].Quantity);
			InventorySlots[i].Quantity -= RemovedAmount;

			// 가방 무게 줄여주기!
			if (ItemInfo)
			{
				CurrentWeight -= (ItemInfo->ItemWeight * RemovedAmount);
				// 소수점 오차 방지
				if (CurrentWeight < 0.0f) CurrentWeight = 0.0f;
			}

			if (InventorySlots[i].Quantity <= 0)
			{
				InventorySlots[i].ItemID = NAME_None;
				InventorySlots[i].Quantity = 0;
			}
			return true;
		}
	}
	return false;
}

void UInventoryComponent::UseItemAtIndex(int32 SlotIndex)
{
	// 유효한 슬롯인지, 아이템이 들어있는지 확인
	if (!InventorySlots.IsValidIndex(SlotIndex) || InventorySlots[SlotIndex].IsEmpty()) return;

	FName ItemID = InventorySlots[SlotIndex].ItemID;
	FItemData* ItemInfo = ItemDataTable->FindRow<FItemData>(ItemID, TEXT("UseItem"));

	if (ItemInfo && ItemInfo->ItemEffectClass)
	{
		AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwner());

		UItemEffectBase* EffectCDO = ItemInfo->ItemEffectClass->GetDefaultObject<UItemEffectBase>();

		// [수정됨] UseItem 호출 시 ItemID를 함께 넘겨줍니다!
		if (EffectCDO && EffectCDO->UseItem(Player, ItemID))
		{
			RemoveItem(ItemID, 1);
		}
	}
}

void UInventoryComponent::UseItemByID(FName UseItemID)
{
	if (UseItemID.IsNone() || !ItemDataTable) return;

	bool bHasItem = false;
	for (int32 i = 0; i < InventorySlots.Num(); i++)
	{
		if (InventorySlots[i].ItemID == UseItemID && InventorySlots[i].Quantity > 0)
		{
			bHasItem = true;
			break;
		}
	}

	if (!bHasItem) return;

	FItemData* ItemInfo = ItemDataTable->FindRow<FItemData>(UseItemID, TEXT("UseItem"));
	if (!ItemInfo) return;

	AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwner());
	if (!Player) return;

	bool bUseSuccess = false;

	switch (ItemInfo->ItemType)
	{
	case EItemType::Consumable:
	case EItemType::Artifact:
	{
		if (ItemInfo->ItemEffectClass)
		{
			UItemEffectBase* EffectCDO = ItemInfo->ItemEffectClass->GetDefaultObject<UItemEffectBase>();
			if (EffectCDO && EffectCDO->UseItem(Player, UseItemID))
			{
				bUseSuccess = true;
			}
		}
		break;
	}

	case EItemType::Throwable:
	{
		// 투척 무기 슬롯에 ID를 등록합니다.
		EquippedThrowableID = UseItemID;

		// 무기를 새로 스폰하여 허리춤(홀스터)에 달아줍니다.
		Player->InitializeWeapon();
		Player->RefreshStudioEquipment();

		bUseSuccess = true;
		break;
	}

	case EItemType::Equipment:
	{
		int32 TargetIndex = -1;
		float TargetDurability = 0.0f;
		for (int32 i = 0; i < InventorySlots.Num(); i++)
		{
			if (InventorySlots[i].ItemID == UseItemID && InventorySlots[i].Quantity > 0)
			{
				TargetIndex = i;
				TargetDurability = InventorySlots[i].CurrentDurability;
				break;
			}
		}

		if (TargetIndex != -1)
		{
			FName OldEquipID = NAME_None;
			float OldDurability = 0.0f;

			if (ItemInfo->EquipType == EEquipType::Helmet)
			{
				OldEquipID = EquippedHelmetID;
				OldDurability = CurrentHelmetDurability;
				EquippedHelmetID = UseItemID;
				CurrentHelmetDurability = TargetDurability;
			}
			else if (ItemInfo->EquipType == EEquipType::Vest)
			{
				OldEquipID = EquippedVestID;
				OldDurability = CurrentVestDurability;
				EquippedVestID = UseItemID;
				CurrentVestDurability = TargetDurability;
			}
			else if (ItemInfo->EquipType == EEquipType::Backpack)
			{
				OldEquipID = EquippedBackpackID;
				// [수정] 가방은 내구도를 안 쓰지만, 스왑할 때 0으로 초기화되는 것을 막기 위해 값을 보존해 줍니다.
				OldDurability = TargetDurability;
				EquippedBackpackID = UseItemID;
			}

			// [추가] 배그 고증: 더 작은 가방/조끼로 교체할 때, 무게가 넘친다면 장착 거부!
			if (!OldEquipID.IsNone())
			{
				if (FItemData* OldItemInfo = ItemDataTable->FindRow<FItemData>(OldEquipID, TEXT("SwapWeightCheck")))
				{
					float NewMaxWeight = MaxWeight - OldItemInfo->StatBonus + ItemInfo->StatBonus;
					if (CurrentWeight > NewMaxWeight)
					{
						if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("용량이 부족하여 교체할 수 없습니다!"));

						// 장착 거부되었으므로 변수들 원상복구
						if (ItemInfo->EquipType == EEquipType::Helmet) { EquippedHelmetID = OldEquipID; CurrentHelmetDurability = OldDurability; }
						else if (ItemInfo->EquipType == EEquipType::Vest) { EquippedVestID = OldEquipID; CurrentVestDurability = OldDurability; }
						else if (ItemInfo->EquipType == EEquipType::Backpack) { EquippedBackpackID = OldEquipID; }
						return;
					}
				}
			}

			if (OldEquipID.IsNone())
			{
				InventorySlots[TargetIndex].Quantity--;
				if (InventorySlots[TargetIndex].Quantity <= 0)
				{
					InventorySlots[TargetIndex].ItemID = NAME_None;
					InventorySlots[TargetIndex].CurrentDurability = 0.0f;
				}
				CurrentWeight = FMath::Max(0.0f, CurrentWeight - ItemInfo->ItemWeight);
				MaxWeight += ItemInfo->StatBonus; // 장비 장착 시 용량 증가
			}
			else
			{
				InventorySlots[TargetIndex].ItemID = OldEquipID;
				InventorySlots[TargetIndex].Quantity = 1;
				InventorySlots[TargetIndex].CurrentDurability = OldDurability;

				if (FItemData* OldItemInfo = ItemDataTable->FindRow<FItemData>(OldEquipID, TEXT("SwapWeight")))
				{
					CurrentWeight = FMath::Max(0.0f, CurrentWeight - ItemInfo->ItemWeight + OldItemInfo->ItemWeight);
					MaxWeight = MaxWeight - OldItemInfo->StatBonus + ItemInfo->StatBonus;
				}
			}

			Player->UpdateEquipmentVisuals(ItemInfo->EquipType, ItemInfo->EquipmentMesh);
			bUseSuccess = true;
		}
		break;
	}

	case EItemType::Weapon:
	{
		int32 TargetIndex = -1;
		for (int32 i = 0; i < InventorySlots.Num(); i++)
		{
			if (InventorySlots[i].ItemID == UseItemID && InventorySlots[i].Quantity > 0)
			{
				TargetIndex = i;
				break;
			}
		}

		if (TargetIndex != -1)
		{
			FName OldEquipID = NAME_None;
			int32 TargetWeaponSlotIndex = 1; // 1: 무기1, 2: 무기2

			// [스마트 장착 로직] 1번 비어있으면 1번, 아니면 2번, 둘다 차있으면 1번을 교체!
			if (EquippedWeapon1ID.IsNone())
			{
				TargetWeaponSlotIndex = 1;
				EquippedWeapon1ID = UseItemID;
			}
			else if (EquippedWeapon2ID.IsNone())
			{
				TargetWeaponSlotIndex = 2;
				EquippedWeapon2ID = UseItemID;
			}
			else
			{
				TargetWeaponSlotIndex = 1; // 꽉 찼을 땐 1번 무기를 교체
				OldEquipID = EquippedWeapon1ID;
				EquippedWeapon1ID = UseItemID;
			}

			// 인벤토리에서 무기 제거 및 교체(스왑) 처리
			if (OldEquipID.IsNone())
			{
				InventorySlots[TargetIndex].Quantity--;
				if (InventorySlots[TargetIndex].Quantity <= 0) InventorySlots[TargetIndex].ItemID = NAME_None;
				CurrentWeight = FMath::Max(0.0f, CurrentWeight - ItemInfo->ItemWeight);
			}
			else
			{
				InventorySlots[TargetIndex].ItemID = OldEquipID;
				InventorySlots[TargetIndex].Quantity = 1;
				if (FItemData* OldItemInfo = ItemDataTable->FindRow<FItemData>(OldEquipID, TEXT("SwapWeight")))
				{
					CurrentWeight = FMath::Max(0.0f, CurrentWeight - ItemInfo->ItemWeight + OldItemInfo->ItemWeight);
				}
			}

			// [핵심] 진짜 무기 액터 스폰 후 WeaponSlots 배열에 등록
			if (ItemInfo->WeaponClass)
			{
				// 기존에 들고 있던 총기 액터 파괴
				if (Player->WeaponSlots.IsValidIndex(TargetWeaponSlotIndex) && Player->WeaponSlots[TargetWeaponSlotIndex])
				{
					Player->WeaponSlots[TargetWeaponSlotIndex]->Destroy();
					Player->WeaponSlots[TargetWeaponSlotIndex] = nullptr;
				}

				// 새 무기 스폰
				FActorSpawnParameters SpawnParams;
				SpawnParams.Owner = Player;
				AWeaponBase* NewWeapon = GetWorld()->SpawnActor<AWeaponBase>(ItemInfo->WeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

				if (NewWeapon)
				{
					NewWeapon->SetActorHiddenInGame(false);
					NewWeapon->SetActorEnableCollision(false); // 플레이어 이동을 방해하지 않게 충돌은 꺼둠
					NewWeapon->HolsterRotationOffset = ItemInfo->HolsterRotationOffset;

					// 슬롯에 장전!
					Player->WeaponSlots[TargetWeaponSlotIndex] = NewWeapon;
					Player->AttachToHolster(TargetWeaponSlotIndex);
				}
			}
			bUseSuccess = true;
		}
		break;
	}

	default:
		break;
	}

	// [중요 수정] 아이템 삭제 처리 방어 코드
	if (bUseSuccess)
	{
		// [핵심] 투척물(Throwable)도 장착 시점에는 삭제하면 안 되므로 예외로 추가!
		if (ItemInfo->ItemType != EItemType::Equipment &&
			ItemInfo->ItemType != EItemType::Weapon &&
			ItemInfo->ItemType != EItemType::Throwable)
		{
			RemoveItem(UseItemID, 1);
		}

		if (Player->bIsInventoryOpen && Player->InventoryWidgetInstance)
		{
			if (UInventoryMainWidget* MainWidget = Cast<UInventoryMainWidget>(Player->InventoryWidgetInstance))
			{
				MainWidget->RefreshInventory();
			}
		}
	}
}

void UInventoryComponent::UnequipItemByID(FName ItemID)
{
	if (ItemID.IsNone() || !ItemDataTable) return;

	AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwner());
	if (!Player) return;

	FItemData* ItemData = ItemDataTable->FindRow<FItemData>(ItemID, TEXT("Unequip"));
	if (!ItemData) return;

	// =========================================================================
	// [핵심 추가] 배틀그라운드 고증: 가방/조끼를 벗었을 때 현재 아이템 무게가 최대 용량을 초과한다면 벗지 못하게 막음!
	// =========================================================================
	if (EquippedBackpackID == ItemID || EquippedVestID == ItemID)
	{
		if (CurrentWeight > (MaxWeight - ItemData->StatBonus))
		{
			// if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("공간이 부족하여 장비를 해제할 수 없습니다! (아이템을 먼저 버리세요)"));
			return; // 해제 취소!
		}
	}

	// 1. 조끼 해제
	if (EquippedVestID == ItemID)
	{
		EquippedVestID = NAME_None;
		MaxWeight -= ItemData->StatBonus;
		Player->UpdateEquipmentVisuals(EEquipType::Vest, nullptr);
	}
	// 2. 헬멧 해제 
	else if (EquippedHelmetID == ItemID)
	{
		EquippedHelmetID = NAME_None;
		Player->UpdateEquipmentVisuals(EEquipType::Helmet, nullptr);
	}
	// 3. [추가] 가방 해제
	else if (EquippedBackpackID == ItemID)
	{
		EquippedBackpackID = NAME_None;
		MaxWeight -= ItemData->StatBonus; // 늘어났던 가방 용량을 다시 뺌
		Player->UpdateEquipmentVisuals(EEquipType::Backpack, nullptr); // 캐릭터 가방 메쉬 숨기기
	}
	else if (EquippedWeapon1ID == ItemID)
	{
		EquippedWeapon1ID = NAME_None;
		if (Player->WeaponSlots.IsValidIndex(1) && Player->WeaponSlots[1])
		{
			// 혹시 지금 손에 들고 있는 총을 벗은 거라면, 기본 권총(0번)을 들게 만듭니다.
			if (Player->GetCurrentWeapon() == Player->WeaponSlots[1])
			{
				// [버그 해결 핵심] 인벤토리가 열려있으면 EquipWeaponSlot이 강제 종료되므로,
				// 잠시 인벤토리 상태를 닫힘(false)으로 속이고 무기를 바꾼 뒤 다시 복구합니다!
				bool bWasOpen = Player->bIsInventoryOpen;
				Player->bIsInventoryOpen = false;

				Player->EquipWeaponSlot(0); // 이제 무사히 0번(권총)으로 교체되고 스튜디오도 갱신됨!

				Player->bIsInventoryOpen = bWasOpen;
			}
			// 권총으로 무사히 교체된 후, 등에 메고 있던 소총을 완전히 파괴합니다.
			Player->WeaponSlots[1]->Destroy();
			Player->WeaponSlots[1] = nullptr;
		}
	}
	// 무기 2번 해제
	else if (EquippedWeapon2ID == ItemID)
	{
		EquippedWeapon2ID = NAME_None;
		if (Player->WeaponSlots.IsValidIndex(2) && Player->WeaponSlots[2])
		{
			if (Player->GetCurrentWeapon() == Player->WeaponSlots[2])
			{
				// 1번 슬롯과 똑같이 안전하게 우회하여 권총 장착!
				bool bWasOpen = Player->bIsInventoryOpen;
				Player->bIsInventoryOpen = false;

				Player->EquipWeaponSlot(0);

				Player->bIsInventoryOpen = bWasOpen;
			}
			Player->WeaponSlots[2]->Destroy();
			Player->WeaponSlots[2] = nullptr;
		}
	}
	else
	{
		return;
	}

	int32 Leftover = AddItem(ItemID, 1);

	if (Leftover > 0)
	{
		// 가방이 꽉 차서 못 들어갔다면 바닥에 떨어뜨립니다!
		//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, TEXT("가방이 꽉 차서 장비를 바닥에 떨어뜨립니다."));

		Player->DropItemToGround(ItemID);
	}
}

int32 UInventoryComponent::GetTotalItemCount(FName ItemID) const
{
	int32 TotalCount = 0;
	for (const FInventorySlot& Slot : InventorySlots)
	{
		if (Slot.ItemID == ItemID)
		{
			TotalCount += Slot.Quantity;
		}
	}
	return TotalCount;
}