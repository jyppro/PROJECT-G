#include "InventoryComponent.h"
#include "Engine/DataTable.h"

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

	// =========================================================
	// 1. 가방에 똑같은 아이템이 이미 있는지 먼저 확인 (겹치기)
	// =========================================================
	for (int32 i = 0; i < InventorySlots.Num(); i++)
	{
		// 빈 슬롯이 아니고, 아이템 ID가 일치한다면?
		if (!InventorySlots[i].IsEmpty() && InventorySlots[i].ItemID == ItemID)
		{
			InventorySlots[i].Quantity += AddableQuantity;
			CurrentWeight += (ItemInfo->ItemWeight * AddableQuantity);

			return Quantity - AddableQuantity;
		}
	}

	// =========================================================
	// 2. 똑같은 아이템이 없다면 빈 슬롯을 찾아서 새로 넣기
	// =========================================================
	for (int32 i = 0; i < InventorySlots.Num(); i++)
	{
		if (InventorySlots[i].IsEmpty())
		{
			InventorySlots[i].ItemID = ItemID;
			InventorySlots[i].Quantity = AddableQuantity;
			CurrentWeight += (ItemInfo->ItemWeight * AddableQuantity);

			return Quantity - AddableQuantity;
		}
	}

	return Quantity;
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