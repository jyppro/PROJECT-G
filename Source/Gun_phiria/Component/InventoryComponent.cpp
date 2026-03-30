#include "InventoryComponent.h"
#include "Engine/DataTable.h"
#include "../Item/ItemEffectBase.h"
#include "../Gun_phiriaCharacter.h"
#include "Blueprint/UserWidget.h" // ADD THIS LINE

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

void UInventoryComponent::UseItemAtIndex(int32 SlotIndex)
{
	// 유효한 슬롯인지, 아이템이 들어있는지 확인
	if (!InventorySlots.IsValidIndex(SlotIndex) || InventorySlots[SlotIndex].IsEmpty()) return;

	FName ItemID = InventorySlots[SlotIndex].ItemID;
	FItemData* ItemInfo = ItemDataTable->FindRow<FItemData>(ItemID, TEXT("UseItem"));

	if (ItemInfo && ItemInfo->ItemEffectClass)
	{
		// 캐릭터 가져오기
		AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwner());

		// C++의 마법: 클래스의 '기본 객체(CDO)'를 가져와서 즉시 함수 실행! (메모리 생성/삭제 비용 0)
		UItemEffectBase* EffectCDO = ItemInfo->ItemEffectClass->GetDefaultObject<UItemEffectBase>();

		if (EffectCDO && EffectCDO->UseItem(Player))
		{
			// 사용에 성공(true)했다면 아이템 1개 소모
			RemoveItem(ItemID, 1);

			// UI 갱신 신호 (필요에 따라 블루프린트나 델리게이트 호출)
			// 예: Player->InventoryWidgetInstance->RefreshInventory();
		}
	}
}

void UInventoryComponent::UseItemByID(FName UseItemID)
{
	if (UseItemID.IsNone() || !ItemDataTable) return;

	// 1. 가방 안에 진짜 이 아이템이 있는지 먼저 검사! (개수가 0보다 큰지)
	bool bHasItem = false;
	for (int32 i = 0; i < InventorySlots.Num(); i++)
	{
		if (InventorySlots[i].ItemID == UseItemID && InventorySlots[i].Quantity > 0)
		{
			bHasItem = true;
			break;
		}
	}

	if (!bHasItem) return; // 가방에 없으면 실행 안 함

	// 2. 데이터 테이블에서 효과 가져오기
	FItemData* ItemInfo = ItemDataTable->FindRow<FItemData>(UseItemID, TEXT("UseItem"));
	if (ItemInfo && ItemInfo->ItemEffectClass)
	{
		AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwner());
		UItemEffectBase* EffectCDO = ItemInfo->ItemEffectClass->GetDefaultObject<UItemEffectBase>();

		// 3. 효과 실행 (구급상자면 체력 80% 회복 로직이 돎)
		if (EffectCDO && EffectCDO->UseItem(Player))
		{
			// 성공했다면 인벤토리에서 아이템 1개 삭제! (RemoveItem 함수가 구현되어 있다고 가정)
			// (만약 RemoveItem이 없다면 직접 해당 슬롯의 Quantity를 1 빼주면 돼!)
			RemoveItem(UseItemID, 1);

			// UI 새로고침 신호
			if (Player && Player->bIsInventoryOpen && Player->InventoryWidgetInstance)
			{
				UFunction* RefreshFunc = Player->InventoryWidgetInstance->FindFunction(FName("RefreshInventory"));
				if (RefreshFunc) Player->InventoryWidgetInstance->ProcessEvent(RefreshFunc, nullptr);
			}
		}
	}
}