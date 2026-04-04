#include "InventoryMainWidget.h"
#include "Components/ScrollBox.h"
#include "TimerManager.h"
#include "ItemSlotWidget.h" 
#include "../Gun_phiriaCharacter.h"
#include "../Item/PickupItemBase.h"
#include "../component/InventoryComponent.h"
#include "ItemTooltipWidget.h"
#include "ItemDragOperation.h"
#include "DropZoneWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "DragVisualWidget.h"
#include "Blueprint/SlateBlueprintLibrary.h"

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
	InventoryScrollBox->ClearChildren();

	AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwningPlayerPawn());
	if (!Player || !Player->PlayerInventory) return;

	const TArray<FInventorySlot>& Slots = Player->PlayerInventory->InventorySlots;

	for (const FInventorySlot& InventorySlot : Slots)
	{
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
	AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwningPlayerPawn());
	if (!Player || !Player->PlayerInventory) return;

	UInventoryComponent* Inventory = Player->PlayerInventory;

	// --- [1] 조끼 (Vest) ---
	if (WBP_VestSlot)
	{
		if (Inventory->EquippedVestID.IsNone()) WBP_VestSlot->SetItemInfo(NAME_None, 0);
		else WBP_VestSlot->SetItemInfo(Inventory->EquippedVestID, 1);
	}

	// --- [2] 헬멧 (Helmet) ---
	if (WBP_HelmetSlot)
	{
		// 네 InventoryComponent에 있는 헬멧 ID 변수명을 사용해! (예: EquippedHelmetID)
		if (Inventory->EquippedHelmetID.IsNone()) WBP_HelmetSlot->SetItemInfo(NAME_None, 0);
		else WBP_HelmetSlot->SetItemInfo(Inventory->EquippedHelmetID, 1);
	}

	// --- [3] 가방 (Backpack) ---
	if (WBP_BackpackSlot)
	{
		// 네 InventoryComponent에 있는 가방 ID 변수명을 사용해! (예: EquippedBackpackID)
		if (Inventory->EquippedBackpackID.IsNone()) WBP_BackpackSlot->SetItemInfo(NAME_None, 0);
		else WBP_BackpackSlot->SetItemInfo(Inventory->EquippedBackpackID, 1);
	}
}

bool UInventoryMainWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);

	UItemDragOperation* DragOp = Cast<UItemDragOperation>(InOperation);
	if (!DragOp) return false;

	FVector2D DropPosition = InDragDropEvent.GetScreenSpacePosition();
	EDropZoneType TargetZone = EDropZoneType::Nearby;

	if (InventoryScrollBox && InventoryScrollBox->GetCachedGeometry().IsUnderLocation(DropPosition))
	{
		TargetZone = EDropZoneType::Backpack;
	}
	// =========================================================================
	// [수정된 부분] 헬멧과 가방 슬롯 영역까지 모두 검사하도록 조건 추가!
	// =========================================================================
	else if ((WBP_VestSlot && WBP_VestSlot->GetCachedGeometry().IsUnderLocation(DropPosition)) ||
		(WBP_HelmetSlot && WBP_HelmetSlot->GetCachedGeometry().IsUnderLocation(DropPosition)) ||
		(WBP_BackpackSlot && WBP_BackpackSlot->GetCachedGeometry().IsUnderLocation(DropPosition)) ||
		(IMG_CharacterPreview && IMG_CharacterPreview->GetCachedGeometry().IsUnderLocation(DropPosition)))
	{
		TargetZone = EDropZoneType::Equipment;
	}

	HandleItemDrop(DragOp, TargetZone);

	return true;
}

void UInventoryMainWidget::HandleItemDrop(UItemDragOperation* Operation, EDropZoneType TargetZone)
{
	AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwningPlayerPawn());
	if (!Player || !Player->PlayerInventory || !Player->PlayerInventory->ItemDataTable) return;

	FName ItemID = Operation->DraggedItemID;

	EDropZoneType SourceZone;
	if (Operation->bIsFromGround) SourceZone = EDropZoneType::Nearby;
	else if (Operation->bIsFromEquip) SourceZone = EDropZoneType::Equipment;
	else SourceZone = EDropZoneType::Backpack;

	if (SourceZone == TargetZone) return;

	if (TargetZone == EDropZoneType::Equipment)
	{
		FItemData* ItemData = Player->PlayerInventory->ItemDataTable->FindRow<FItemData>(ItemID, TEXT("DropTypeCheck"));

		if (!ItemData || ItemData->ItemType != EItemType::Equipment)
		{
			return;
		}
	}

	if (SourceZone == EDropZoneType::Nearby && TargetZone == EDropZoneType::Backpack)
	{
		Player->PlayerInventory->AddItem(ItemID, 1);
		if (Operation->DraggedActor) Operation->DraggedActor->Destroy();
	}
	else if (SourceZone == EDropZoneType::Nearby && TargetZone == EDropZoneType::Equipment)
	{
		Player->PlayerInventory->AddItem(ItemID, 1);
		Player->PlayerInventory->UseItemByID(ItemID);
		if (Operation->DraggedActor) Operation->DraggedActor->Destroy();
	}
	else if (SourceZone == EDropZoneType::Backpack && TargetZone == EDropZoneType::Nearby)
	{
		Player->DropItemToGround(ItemID);
	}
	else if (SourceZone == EDropZoneType::Backpack && TargetZone == EDropZoneType::Equipment)
	{
		Player->PlayerInventory->UseItemByID(ItemID);
	}
	else if (SourceZone == EDropZoneType::Equipment && TargetZone == EDropZoneType::Nearby)
	{
		Player->PlayerInventory->UnequipItemByID(ItemID);
		Player->DropItemToGround(ItemID);
	}
	else if (SourceZone == EDropZoneType::Equipment && TargetZone == EDropZoneType::Backpack)
	{
		Player->PlayerInventory->UnequipItemByID(ItemID);
	}

	RefreshInventory();
	ForceNearbyRefresh();
}

void UInventoryMainWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragCancelled(InDragDropEvent, InOperation);

	UItemDragOperation* DragOp = Cast<UItemDragOperation>(InOperation);
	if (DragOp)
	{
		// 드래그가 취소되면(허공에 놓으면) 바닥에 버리는 로직
		HandleItemDrop(DragOp, EDropZoneType::Nearby);
	}
}