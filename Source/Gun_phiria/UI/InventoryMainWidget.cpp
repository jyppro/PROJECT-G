#include "InventoryMainWidget.h"
#include "Components/ScrollBox.h"
#include "TimerManager.h"
#include "ItemSlotWidget.h" // SetItemInfo를 호출하기 위해 필요
#include "../Gun_phiriaCharacter.h"
#include "../Item/PickupItemBase.h"
#include "../component/InventoryComponent.h"

void UInventoryMainWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 인벤토리가 열릴 때 가방과 주변 아이템을 즉시 새로고침
	RefreshInventory();
	ForceNearbyRefresh();

	GetWorld()->GetTimerManager().SetTimer(NearbyCheckTimer, this, &UInventoryMainWidget::CheckNearbyItems, 0.2f, true);
}

void UInventoryMainWidget::NativeDestruct()
{
	Super::NativeDestruct();
	GetWorld()->GetTimerManager().ClearTimer(NearbyCheckTimer);
}

// 1. 가방(인벤토리) 아이템 리스트 갱신
void UInventoryMainWidget::RefreshInventory()
{
	if (!InventoryScrollBox || !ItemSlotWidgetClass) return;

	// 기존 리스트 초기화
	InventoryScrollBox->ClearChildren();

	AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwningPlayerPawn());
	if (!Player || !Player->PlayerInventory) return;

	// 인벤토리 컴포넌트의 슬롯 배열을 순회
	const TArray<FInventorySlot>& Slots = Player->PlayerInventory->InventorySlots;

	for (const FInventorySlot& InventorySlot : Slots)
	{
		// 아이템이 들어있는 슬롯만 위젯 생성
		if (!InventorySlot.IsEmpty())
		{
			UItemSlotWidget* NewSlot = CreateWidget<UItemSlotWidget>(this, ItemSlotWidgetClass);
			if (NewSlot)
			{
				NewSlot->SetItemInfo(InventorySlot.ItemID, InventorySlot.Quantity);
				InventoryScrollBox->AddChild(NewSlot);
			}
		}
	}
}

// 2. 주변 아이템 리스트 갱신 (캐릭터의 GetNearbyItems 결과 사용)
void UInventoryMainWidget::UpdateNearbyUI(const TArray<APickupItemBase*>& NearbyItems)
{
	if (!VicinityScrollBox || !ItemSlotWidgetClass) return;

	VicinityScrollBox->ClearChildren();

	for (APickupItemBase* Item : NearbyItems)
	{
		if (Item)
		{
			UItemSlotWidget* NewSlot = CreateWidget<UItemSlotWidget>(this, ItemSlotWidgetClass);
			if (NewSlot)
			{
				// PickupItemBase에 있는 ItemID와 Quantity 정보를 가져와 설정
				NewSlot->SetItemInfo(Item->ItemID, Item->Quantity); // 변수명은 본인 프로젝트에 맞춰 확인
				VicinityScrollBox->AddChild(NewSlot);
			}
		}
	}
}

void UInventoryMainWidget::CheckNearbyItems()
{
	if (AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwningPlayerPawn()))
	{
		TArray<APickupItemBase*> NearbyItems = Player->GetNearbyItems();

		if (NearbyItems.Num() != LastNearbyCount)
		{
			LastNearbyCount = NearbyItems.Num();
			UpdateNearbyUI(NearbyItems);
		}
	}
}

void UInventoryMainWidget::ForceNearbyRefresh()
{
	LastNearbyCount = -1;
	CheckNearbyItems();
}