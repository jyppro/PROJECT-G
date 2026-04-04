#include "InventoryComponent.h"
#include "Engine/DataTable.h"
#include "../Item/ItemEffectBase.h"
#include "../Gun_phiriaCharacter.h"
#include "Blueprint/UserWidget.h" // ADD THIS LINE
#include "../UI/InventoryMainWidget.h"

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

	FItemData* ItemInfo = ItemDataTable->FindRow<FItemData>(ItemID, TEXT("AddItem_WeightCheck"));
	if (!ItemInfo) return Quantity;

	int32 AddableQuantity = Quantity;
	if (ItemInfo->ItemWeight > 0.0f)
	{
		float AvailableWeight = MaxWeight - CurrentWeight;
		if (AvailableWeight < ItemInfo->ItemWeight) return Quantity;

		int32 MaxAffordable = FMath::FloorToInt(AvailableWeight / ItemInfo->ItemWeight);
		AddableQuantity = FMath::Min(Quantity, MaxAffordable);
	}

	if (AddableQuantity <= 0) return Quantity;

	int32 RemainingToAdd = AddableQuantity;

	// =========================================================
	// 1. 가방에 똑같은 아이템이 이미 있는지 확인 (겹치기 시도)
	// 장비처럼 MaxStackSize가 1인 아이템은 이 과정을 건너뜁니다.
	// =========================================================
	if (ItemInfo->MaxStackSize > 1)
	{
		for (int32 i = 0; i < InventorySlots.Num(); i++)
		{
			if (!InventorySlots[i].IsEmpty() && InventorySlots[i].ItemID == ItemID)
			{
				int32 SpaceLeft = ItemInfo->MaxStackSize - InventorySlots[i].Quantity;
				if (SpaceLeft > 0)
				{
					int32 AmountToAdd = FMath::Min(RemainingToAdd, SpaceLeft);
					InventorySlots[i].Quantity += AmountToAdd;
					CurrentWeight += (ItemInfo->ItemWeight * AmountToAdd);
					RemainingToAdd -= AmountToAdd;

					if (RemainingToAdd <= 0) return Quantity - AddableQuantity;
				}
			}
		}
	}

	// =========================================================
	// 2. 그래도 남은 수량이 있다면 빈 슬롯을 찾아서 새로 넣기
	// (장비 아이템은 항상 여기로 와서 빈 슬롯을 차지합니다)
	// =========================================================
	for (int32 i = 0; i < InventorySlots.Num(); i++)
	{
		if (InventorySlots[i].IsEmpty())
		{
			InventorySlots[i].ItemID = ItemID;

			// 이 슬롯에 넣을 개수 (최대 스택을 넘지 않도록)
			int32 AmountToAdd = FMath::Min(RemainingToAdd, ItemInfo->MaxStackSize);
			InventorySlots[i].Quantity = AmountToAdd;

			// 획득 시 최대 내구도로 초기화
			InventorySlots[i].CurrentDurability = ItemInfo->MaxDurability;

			CurrentWeight += (ItemInfo->ItemWeight * AmountToAdd);
			RemainingToAdd -= AmountToAdd;

			if (RemainingToAdd <= 0) return Quantity - AddableQuantity;
		}
	}

	// 가방 칸이 부족해서 남은 수량 반환
	return (Quantity - AddableQuantity) + RemainingToAdd;
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
			// (참고) 진통제처럼 캐스팅이 있는 아이템은 여기서 즉시 삭제되지 않도록 
			// ItemEffect 내부에서 false를 반환하고, 캐스팅 성공 시 ItemEffect 내부에서 RemoveItem을 호출하게 됩니다.
			// 만약 캐스팅 없이 즉시 사용되는 아이템(예: 사과)라면 Effect 안에서 true를 반환하여 여기서 삭제됩니다.
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
	case EItemType::Throwable:
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
		break;

	default:
		break;
	}

	if (bUseSuccess)
	{
		if (ItemInfo->ItemType != EItemType::Equipment)
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
	else
	{
		return;
	}

	// 벗은 아이템을 가방에 추가 
	AddItem(ItemID, 1);
}