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

	// 1. 가방 안에 진짜 이 아이템이 있는지 먼저 검사!
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

	// 2. 데이터 테이블에서 아이템 정보 가져오기
	FItemData* ItemInfo = ItemDataTable->FindRow<FItemData>(UseItemID, TEXT("UseItem"));
	if (!ItemInfo) return;

	AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwner());
	if (!Player) return;

	bool bUseSuccess = false;

	// 3. 아이템 타입에 따른 분기 처리
	switch (ItemInfo->ItemType)
	{
	case EItemType::Consumable:
	case EItemType::Throwable:
	case EItemType::Artifact:
	{
		if (ItemInfo->ItemEffectClass)
		{
			UItemEffectBase* EffectCDO = ItemInfo->ItemEffectClass->GetDefaultObject<UItemEffectBase>();

			// [수정됨] UseItem 호출 시 UseItemID를 함께 넘겨줍니다!
			if (EffectCDO && EffectCDO->UseItem(Player, UseItemID))
			{
				bUseSuccess = true;
			}
		}
		break;
	}

	case EItemType::Equipment:
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan, TEXT("장비 장착 시도!"));
		break;
	}

	case EItemType::Weapon:
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("무기 장착 시도!"));
		break;
	}

	default:
		break;
	}

	// 4. 사용/장착에 성공했다면 아이템을 소모하고 UI를 갱신합니다.
	if (bUseSuccess)
	{
		// 이 부분은 캐스팅 없이 즉시 사용(UseItem이 true 반환)되는 아이템을 위한 로직입니다.
		RemoveItem(UseItemID, 1);

		if (Player->bIsInventoryOpen && Player->InventoryWidgetInstance)
		{
			if (UInventoryMainWidget* MainWidget = Cast<UInventoryMainWidget>(Player->InventoryWidgetInstance))
			{
				MainWidget->RefreshInventory();
			}
		}
	}
}