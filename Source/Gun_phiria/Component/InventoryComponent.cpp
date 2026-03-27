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
	// 데이터 테이블이 없거나 넣을 개수가 없으면 실패
	if (Quantity <= 0 || ItemID.IsNone() || !ItemDataTable) return Quantity;

	// 1. 데이터 테이블에서 이 아이템의 정보를 찾아옵니다 (무게 확인용)
	FItemData* ItemInfo = ItemDataTable->FindRow<FItemData>(ItemID, TEXT("AddItem"));
	if (!ItemInfo) return Quantity;

	// 2. 무게 계산: 내가 넣을 수 있는 최대 개수 파악
	int32 AddableQuantity = Quantity;
	if (ItemInfo->ItemWeight > 0.0f)
	{
		float AvailableWeight = MaxWeight - CurrentWeight;

		// 넣을 수 있는 여유 무게가 아예 없다면 하나도 못 넣고 튕겨냄
		if (AvailableWeight < ItemInfo->ItemWeight) return Quantity;

		// 여유 무게 / 아이템 무게 = 담을 수 있는 개수
		int32 MaxAffordable = FMath::FloorToInt(AvailableWeight / ItemInfo->ItemWeight);

		// 원래 주우려던 개수와 가방이 허락하는 개수 중 작은 값을 선택
		AddableQuantity = FMath::Min(Quantity, MaxAffordable);
	}

	// 3. 실제 가방 빈 슬롯에 아이템 넣기 (간단화된 로직)
	for (int32 i = 0; i < InventorySlots.Num(); i++)
	{
		if (InventorySlots[i].IsEmpty())
		{
			InventorySlots[i].ItemID = ItemID;
			InventorySlots[i].Quantity = AddableQuantity;

			// 4. 가방의 현재 무게를 증가시킵니다!
			CurrentWeight += (ItemInfo->ItemWeight * AddableQuantity);

			// 다 넣었다면 0, 가방 무게 때문에 덜 넣었다면 남은 개수를 반환 (땅에 다시 떨구기 위해)
			return Quantity - AddableQuantity;
		}
	}

	// 슬롯 칸수(20칸) 자체가 꽉 차서 못 넣음
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