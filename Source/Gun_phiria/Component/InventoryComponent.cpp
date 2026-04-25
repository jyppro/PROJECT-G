#include "InventoryComponent.h"
#include "../Item/ItemEffectBase.h"
#include "../Gun_phiriaCharacter.h"
#include "Blueprint/UserWidget.h"
#include "../UI/InventoryMainWidget.h"
#include "../Weapon/WeaponBase.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	InventorySlots.SetNum(MaxSlots);
}

int32 UInventoryComponent::AddItem(FName ItemID, int32 Quantity)
{
	if (Quantity <= 0 || ItemID.IsNone() || !ItemDataTable) return Quantity;

	FItemData* ItemInfo = ItemDataTable->FindRow<FItemData>(ItemID, TEXT("AddItem"));
	if (!ItemInfo) return Quantity;

	int32 RemainingToAdd = Quantity;

	// [리팩토링] 무게 계산 로직을 람다로 분리하여 중복 제거
	auto GetAllowedAmount = [&](int32 MaxCanAdd) -> int32 {
		if (ItemInfo->ItemWeight <= 0.f) return MaxCanAdd;
		int32 WeightPossible = FMath::FloorToInt((MaxWeight - CurrentWeight) / ItemInfo->ItemWeight);
		return FMath::Min(MaxCanAdd, WeightPossible);
		};

	// 1. 기존 슬롯에 겹치기
	if (ItemInfo->MaxStackSize > 1)
	{
		for (FInventorySlot& Slot : InventorySlots)
		{
			if (!Slot.IsEmpty() && Slot.ItemID == ItemID)
			{
				int32 SpaceLeft = ItemInfo->MaxStackSize - Slot.Quantity;
				if (SpaceLeft > 0)
				{
					int32 AmountToAdd = GetAllowedAmount(FMath::Min(RemainingToAdd, SpaceLeft));
					if (AmountToAdd > 0)
					{
						Slot.Quantity += AmountToAdd;
						CurrentWeight += (ItemInfo->ItemWeight * AmountToAdd);
						RemainingToAdd -= AmountToAdd;
					}
					if (RemainingToAdd <= 0) return RemainingToAdd;
				}
			}
		}
	}

	// 2. 새 슬롯에 넣기
	for (FInventorySlot& Slot : InventorySlots)
	{
		if (Slot.IsEmpty())
		{
			int32 AmountToAdd = GetAllowedAmount(FMath::Min(RemainingToAdd, ItemInfo->MaxStackSize));
			if (AmountToAdd > 0)
			{
				Slot.ItemID = ItemID;
				Slot.Quantity = AmountToAdd;
				Slot.CurrentDurability = ItemInfo->MaxDurability;

				CurrentWeight += (ItemInfo->ItemWeight * AmountToAdd);
				RemainingToAdd -= AmountToAdd;
			}
			if (RemainingToAdd <= 0) return RemainingToAdd;
		}
	}

	return RemainingToAdd;
}

bool UInventoryComponent::RemoveItem(FName ItemID, int32 Quantity)
{
	if (Quantity <= 0 || ItemID.IsNone() || !ItemDataTable) return false;

	FItemData* ItemInfo = ItemDataTable->FindRow<FItemData>(ItemID, TEXT("RemoveItem"));

	for (FInventorySlot& Slot : InventorySlots)
	{
		if (Slot.ItemID == ItemID)
		{
			int32 RemovedAmount = FMath::Min(Quantity, Slot.Quantity);
			Slot.Quantity -= RemovedAmount;

			if (ItemInfo)
			{
				CurrentWeight = FMath::Max(0.0f, CurrentWeight - (ItemInfo->ItemWeight * RemovedAmount));
			}

			if (Slot.Quantity <= 0)
			{
				Slot.ItemID = NAME_None;
				Slot.Quantity = 0;
			}
			return true;
		}
	}
	return false;
}

void UInventoryComponent::UseItemAtIndex(int32 SlotIndex)
{
	if (!InventorySlots.IsValidIndex(SlotIndex) || InventorySlots[SlotIndex].IsEmpty()) return;

	FName ItemID = InventorySlots[SlotIndex].ItemID;
	FItemData* ItemInfo = ItemDataTable->FindRow<FItemData>(ItemID, TEXT("UseItem"));

	if (ItemInfo && ItemInfo->ItemEffectClass)
	{
		AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwner());
		UItemEffectBase* EffectCDO = ItemInfo->ItemEffectClass->GetDefaultObject<UItemEffectBase>();

		if (EffectCDO && EffectCDO->UseItem(Player, ItemID))
		{
			RemoveItem(ItemID, 1);
		}
	}
}

void UInventoryComponent::UseItemByID(FName UseItemID)
{
	if (UseItemID.IsNone() || !ItemDataTable) return;

	// [리팩토링] for문 대신 IndexOfByPredicate를 사용하여 코드 간결화 및 탐색 중복 제거
	int32 TargetIndex = InventorySlots.IndexOfByPredicate([&](const FInventorySlot& Slot) {
		return Slot.ItemID == UseItemID && Slot.Quantity > 0;
		});

	if (TargetIndex == INDEX_NONE) return;

	FItemData* ItemInfo = ItemDataTable->FindRow<FItemData>(UseItemID, TEXT("UseItem"));
	AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwner());
	if (!ItemInfo || !Player) return;

	bool bUseSuccess = false;

	switch (ItemInfo->ItemType)
	{
	case EItemType::Consumable:
	case EItemType::Artifact:
	{
		if (ItemInfo->ItemEffectClass)
		{
			UItemEffectBase* EffectCDO = ItemInfo->ItemEffectClass->GetDefaultObject<UItemEffectBase>();
			if (EffectCDO && EffectCDO->UseItem(Player, UseItemID)) bUseSuccess = true;
		}
		break;
	}
	case EItemType::Throwable:
	{
		if (!EquippedThrowableID.IsNone())
		{
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, TEXT("Already Equip Grenade"));
			return;
		}

		RemoveItem(UseItemID, 1);
		EquippedThrowableID = UseItemID;

		Player->UpdateThrowableSlot();
		Player->RefreshStudioEquipment();
		bUseSuccess = true;
		break;
	}
	case EItemType::Equipment:
	{
		FName OldEquipID = NAME_None;
		float OldDurability = 0.0f;
		float TargetDurability = InventorySlots[TargetIndex].CurrentDurability;

		if (ItemInfo->EquipType == EEquipType::Helmet)
		{
			OldEquipID = EquippedHelmetID; OldDurability = CurrentHelmetDurability;
			EquippedHelmetID = UseItemID; CurrentHelmetDurability = TargetDurability;
		}
		else if (ItemInfo->EquipType == EEquipType::Vest)
		{
			OldEquipID = EquippedVestID; OldDurability = CurrentVestDurability;
			EquippedVestID = UseItemID; CurrentVestDurability = TargetDurability;
		}
		else if (ItemInfo->EquipType == EEquipType::Backpack)
		{
			OldEquipID = EquippedBackpackID; OldDurability = TargetDurability; // 가방 스왑 시 내구도 보존
			EquippedBackpackID = UseItemID;
		}

		// 무게 초과 체크
		if (!OldEquipID.IsNone())
		{
			if (FItemData* OldItemInfo = ItemDataTable->FindRow<FItemData>(OldEquipID, TEXT("SwapWeightCheck")))
			{
				float NewMaxWeight = MaxWeight - OldItemInfo->StatBonus + ItemInfo->StatBonus;
				if (CurrentWeight > NewMaxWeight)
				{
					if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("용량이 부족하여 교체할 수 없습니다!"));

					// 원상복구
					if (ItemInfo->EquipType == EEquipType::Helmet) { EquippedHelmetID = OldEquipID; CurrentHelmetDurability = OldDurability; }
					else if (ItemInfo->EquipType == EEquipType::Vest) { EquippedVestID = OldEquipID; CurrentVestDurability = OldDurability; }
					else if (ItemInfo->EquipType == EEquipType::Backpack) { EquippedBackpackID = OldEquipID; }
					return;
				}
			}
		}

		// 인벤토리 슬롯 처리
		if (OldEquipID.IsNone())
		{
			InventorySlots[TargetIndex].Quantity--;
			if (InventorySlots[TargetIndex].Quantity <= 0)
			{
				InventorySlots[TargetIndex].ItemID = NAME_None;
				InventorySlots[TargetIndex].CurrentDurability = 0.0f;
			}
			CurrentWeight = FMath::Max(0.0f, CurrentWeight - ItemInfo->ItemWeight);
			MaxWeight += ItemInfo->StatBonus;
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
		break;
	}
	case EItemType::Weapon:
	{
		FName OldEquipID = NAME_None;
		int32 TargetWeaponSlotIndex = 1;

		if (EquippedWeapon1ID.IsNone()) { TargetWeaponSlotIndex = 1; EquippedWeapon1ID = UseItemID; }
		else if (EquippedWeapon2ID.IsNone()) { TargetWeaponSlotIndex = 2; EquippedWeapon2ID = UseItemID; }
		else { TargetWeaponSlotIndex = 1; OldEquipID = EquippedWeapon1ID; EquippedWeapon1ID = UseItemID; }

		// 인벤토리 교체
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

		if (ItemInfo->WeaponClass)
		{
			if (Player->WeaponSlots.IsValidIndex(TargetWeaponSlotIndex) && Player->WeaponSlots[TargetWeaponSlotIndex])
			{
				Player->WeaponSlots[TargetWeaponSlotIndex]->Destroy();
				Player->WeaponSlots[TargetWeaponSlotIndex] = nullptr;
			}

			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = Player;
			AWeaponBase* NewWeapon = GetWorld()->SpawnActor<AWeaponBase>(ItemInfo->WeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

			if (NewWeapon)
			{
				NewWeapon->SetActorHiddenInGame(false);
				NewWeapon->SetActorEnableCollision(false);
				NewWeapon->HolsterRotationOffset = ItemInfo->HolsterRotationOffset;

				Player->WeaponSlots[TargetWeaponSlotIndex] = NewWeapon;
				Player->AttachToHolster(TargetWeaponSlotIndex);
			}
		}
		bUseSuccess = true;
		break;
	}
	default:
		break;
	}

	if (bUseSuccess)
	{
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
	FItemData* ItemData = ItemDataTable->FindRow<FItemData>(ItemID, TEXT("Unequip"));
	if (!Player || !ItemData) return;

	// 가방/조끼 용량 체크 (초과 시 해제 불가)
	if (EquippedBackpackID == ItemID || EquippedVestID == ItemID)
	{
		if (CurrentWeight > (MaxWeight - ItemData->StatBonus)) return;
	}

	// [리팩토링] 무기 해제 로직이 완전히 동일하여 람다로 통합
	auto UnequipWeaponSlot = [&](int32 SlotIndex, FName& EquippedSlotID) {
		EquippedSlotID = NAME_None;
		if (Player->WeaponSlots.IsValidIndex(SlotIndex) && Player->WeaponSlots[SlotIndex])
		{
			if (Player->GetCurrentWeapon() == Player->WeaponSlots[SlotIndex])
			{
				// 인벤토리 강제 종료를 막기 위한 우회 처리 (원래 코드 기능 유지)
				bool bWasOpen = Player->bIsInventoryOpen;
				Player->bIsInventoryOpen = false;
				Player->EquipWeaponSlot(0);
				Player->bIsInventoryOpen = bWasOpen;
			}
			Player->WeaponSlots[SlotIndex]->Destroy();
			Player->WeaponSlots[SlotIndex] = nullptr;
		}
		};

	if (EquippedVestID == ItemID)
	{
		EquippedVestID = NAME_None;
		MaxWeight -= ItemData->StatBonus;
		Player->UpdateEquipmentVisuals(EEquipType::Vest, nullptr);
	}
	else if (EquippedHelmetID == ItemID)
	{
		EquippedHelmetID = NAME_None;
		Player->UpdateEquipmentVisuals(EEquipType::Helmet, nullptr);
	}
	else if (EquippedBackpackID == ItemID)
	{
		EquippedBackpackID = NAME_None;
		MaxWeight -= ItemData->StatBonus;
		Player->UpdateEquipmentVisuals(EEquipType::Backpack, nullptr);
	}
	else if (EquippedWeapon1ID == ItemID)
	{
		UnequipWeaponSlot(1, EquippedWeapon1ID);
	}
	else if (EquippedWeapon2ID == ItemID)
	{
		UnequipWeaponSlot(2, EquippedWeapon2ID);
	}
	else
	{
		return; // 일치하는 장착 아이템이 없음
	}

	int32 Leftover = AddItem(ItemID, 1);
	if (Leftover > 0)
	{
		Player->DropItemToGround(ItemID);
	}
}

int32 UInventoryComponent::GetTotalItemCount(FName ItemID) const
{
	int32 TotalCount = 0;
	for (const FInventorySlot& Slot : InventorySlots)
	{
		if (Slot.ItemID == ItemID) TotalCount += Slot.Quantity;
	}
	return TotalCount;
}