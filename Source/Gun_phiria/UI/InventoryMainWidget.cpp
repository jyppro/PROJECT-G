#include "InventoryMainWidget.h"
#include "Components/ScrollBox.h"
#include "TimerManager.h"
#include "ItemSlotWidget.h" // SetItemInfo를 호출하기 위해 필요
#include "../Gun_phiriaCharacter.h"
#include "../Item/PickupItemBase.h"
#include "../component/InventoryComponent.h"
#include "ItemTooltipWidget.h"
#include "ItemDragOperation.h"
#include "DropZoneWidget.h"

void UInventoryMainWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 인벤토리가 열릴 때 가방과 주변 아이템을 즉시 새로고침
	RefreshInventory();
	ForceNearbyRefresh();

	GetWorld()->GetTimerManager().SetTimer(NearbyCheckTimer, this, &UInventoryMainWidget::CheckNearbyItems, 0.2f, true);

	if (TooltipClass && !CachedTooltip)
	{
		CachedTooltip = CreateWidget<UItemTooltipWidget>(this, TooltipClass);
		CachedTooltip->SetVisibility(ESlateVisibility::Collapsed);
	}
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

	HideTooltip();

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

	UpdateEquipmentUI();
}

void UInventoryMainWidget::UpdateNearbyUI(const TArray<APickupItemBase*>& NearbyItems)
{
	if (!VicinityScrollBox || !ItemSlotWidgetClass) return;

	HideTooltip();

	VicinityScrollBox->ClearChildren();

	for (APickupItemBase* Item : NearbyItems)
	{
		if (Item)
		{
			UItemSlotWidget* NewSlot = CreateWidget<UItemSlotWidget>(this, ItemSlotWidgetClass);
			if (NewSlot)
			{
				NewSlot->bIsVicinitySlot = true;

				// 실제 아이템 액터를 슬롯에 기억시킵니다.
				NewSlot->TargetItemActor = Item;

				NewSlot->SetItemInfo(Item->ItemID, Item->Quantity);
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

void UInventoryMainWidget::ShowTooltip(FName ItemID, UDataTable* DataTable)
{
	if (CachedTooltip)
	{
		CachedTooltip->UpdateTooltip(ItemID, DataTable);
		CachedTooltip->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		// 위젯의 툴팁 시스템에 연결 (마우스 위치를 자동으로 따라가게 함)
		SetToolTip(CachedTooltip);
	}
}

void UInventoryMainWidget::HideTooltip()
{
	if (CachedTooltip)
	{
		CachedTooltip->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UInventoryMainWidget::UpdateEquipmentUI()
{
	// 1. 플레이어와 인벤토리 컴포넌트 가져오기
	AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwningPlayerPawn());
	if (!Player || !Player->PlayerInventory) return;

	UInventoryComponent* Inventory = Player->PlayerInventory;

	// 2. 조끼 장착 여부 확인
	if (Inventory->EquippedVestID.IsNone())
	{
		// [조끼를 안 입고 있을 때]
		// 아예 안 보이게 숨기거나, 기본 실루엣 이미지로 되돌리는 처리
		if (IMG_EquippedVest)
		{
			// 빈 슬롯 이미지를 쓴다면 투명도를 조절하거나, 그냥 이미지를 비워버리면 됨
			IMG_EquippedVest->SetBrushFromTexture(nullptr);
			IMG_EquippedVest->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.0f)); // 투명하게 만듦
		}
	}
	else
	{
		// [조끼를 입고 있을 때]
		if (IMG_EquippedVest && Inventory->ItemDataTable)
		{
			// 장착된 조끼의 ID로 데이터 테이블에서 아이콘(ItemIcon) 가져오기
			FItemData* VestData = Inventory->ItemDataTable->FindRow<FItemData>(Inventory->EquippedVestID, TEXT("UI_VestUpdate"));
			if (VestData && VestData->ItemIcon)
			{
				IMG_EquippedVest->SetBrushFromTexture(VestData->ItemIcon);
				IMG_EquippedVest->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 1.0f)); // 불투명하게 보이도록 복구
			}
		}
	}
}

//bool UInventoryMainWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
//{
//	// 1. 부모 함수 먼저 실행
//	Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
//
//	// 2. 넘어온 운반책이 우리가 만든 'UItemDragOperation'이 맞는지 확인 (형변환)
//	UItemDragOperation* ItemDrag = Cast<UItemDragOperation>(InOperation);
//	if (ItemDrag)
//	{
//		AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwningPlayerPawn());
//		if (Player && Player->PlayerInventory)
//		{
//			if (ItemDrag->bIsFromGround)
//			{
//				Player->PlayerInventory->AddItem(ItemDrag->DraggedItemID, 1);
//
//				// [추가] 맵에 있는 실제 아이템 액터 파괴
//				if (ItemDrag->DraggedActor)
//				{
//					ItemDrag->DraggedActor->Destroy();
//				}
//				// [추가] 주변 아이템 목록 UI 새로고침
//				ForceNearbyRefresh();
//			}
//
//			// 4. 아이템 장착(또는 사용) C++ 로직 실행!
//			Player->PlayerInventory->UseItemByID(ItemDrag->DraggedItemID);
//
//			// 5. 드롭 처리가 성공적으로 끝났음을 엔진에 알림
//			return true;
//		}
//	}
//
//	// 우리가 원하는 드래그 객체가 아니면 실패 처리
//	return false;
//}

void UInventoryMainWidget::HandleItemDrop(UItemDragOperation* Operation, EDropZoneType TargetZone)
{
	AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwningPlayerPawn());
	if (!Player || !Player->PlayerInventory) return;

	FName ItemID = Operation->DraggedItemID;

	// 1. 어디서 끌고 왔는가? (출처 파악)
	EDropZoneType SourceZone;
	if (Operation->bIsFromGround) SourceZone = EDropZoneType::Nearby;
	else if (Operation->bIsFromEquip) SourceZone = EDropZoneType::Equipment;
	else SourceZone = EDropZoneType::Backpack;

	// 2. 같은 구역 안에서 던졌으면 무시
	if (SourceZone == TargetZone) return;

	// =====================================
	// 3. 6가지 로직 분기 (완성본)
	// =====================================

	if (SourceZone == EDropZoneType::Nearby && TargetZone == EDropZoneType::Backpack)
	{
		// [바닥 -> 가방] : 줍기
		Player->PlayerInventory->AddItem(ItemID, 1);
		if (Operation->DraggedActor) Operation->DraggedActor->Destroy();
	}
	else if (SourceZone == EDropZoneType::Nearby && TargetZone == EDropZoneType::Equipment)
	{
		// [바닥 -> 장착] : 주워서 바로 장착
		Player->PlayerInventory->AddItem(ItemID, 1);
		Player->PlayerInventory->UseItemByID(ItemID);
		if (Operation->DraggedActor) Operation->DraggedActor->Destroy();
	}
	else if (SourceZone == EDropZoneType::Backpack && TargetZone == EDropZoneType::Nearby)
	{
		// [가방 -> 바닥] : 버리기
		Player->DropItemToGround(ItemID);
	}
	else if (SourceZone == EDropZoneType::Backpack && TargetZone == EDropZoneType::Equipment)
	{
		// [가방 -> 장착] : 입기
		Player->PlayerInventory->UseItemByID(ItemID);
	}
	else if (SourceZone == EDropZoneType::Equipment && TargetZone == EDropZoneType::Nearby)
	{
		// [장착 -> 바닥] : 벗어서 바로 버리기
		Player->PlayerInventory->UnequipItemByID(ItemID);
		Player->DropItemToGround(ItemID);
	}
	else if (SourceZone == EDropZoneType::Equipment && TargetZone == EDropZoneType::Backpack)
	{
		// [장착 -> 가방] : 벗어서 가방에 넣기
		Player->PlayerInventory->UnequipItemByID(ItemID);
	}

	// 4. 모든 처리가 끝나면 화면 새로고침
	RefreshInventory();
	ForceNearbyRefresh();
}