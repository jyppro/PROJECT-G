#include "InventoryMainWidget.h"
#include "Components/ScrollBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/PanelWidget.h"
#include "TimerManager.h"
#include "ItemSlotWidget.h" 
#include "../Gun_phiriaCharacter.h"
#include "../Item/PickupItemBase.h"
#include "../component/InventoryComponent.h"
#include "ItemTooltipWidget.h"
#include "ItemDragOperation.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "DragVisualWidget.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "../NPC/ShopNPC.h"
#include "QuantityPopupWidget.h"

void UInventoryMainWidget::NativeConstruct()
{
	Super::NativeConstruct();

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

void UInventoryMainWidget::RefreshInventory()
{
	if (!InventoryScrollBox || !ItemSlotWidgetClass) return;

	HideTooltip();
	InventoryScrollBox->ClearChildren();

	AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwningPlayerPawn());
	if (!Player || !Player->PlayerInventory) return;

	for (const FInventorySlot& InvSlot : Player->PlayerInventory->InventorySlots)
	{
		if (!InvSlot.IsEmpty())
		{
			if (UItemSlotWidget* NewSlot = CreateWidget<UItemSlotWidget>(this, ItemSlotWidgetClass))
			{
				NewSlot->SetItemInfo(InvSlot.ItemID, InvSlot.Quantity);
				InventoryScrollBox->AddChild(NewSlot);
			}
		}
	}

	UpdateEquipmentUI();
	UpdateCurrencyUI();
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
			if (UItemSlotWidget* NewSlot = CreateWidget<UItemSlotWidget>(this, ItemSlotWidgetClass))
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
	if (CachedTooltip) CachedTooltip->SetVisibility(ESlateVisibility::Collapsed);
}

void UInventoryMainWidget::UpdateEquipmentUI()
{
	AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwningPlayerPawn());
	if (!Player || !Player->PlayerInventory) return;

	UInventoryComponent* Inv = Player->PlayerInventory;

	// [리팩토링] 장착 슬롯 업데이트 람다 함수 (중복 제거)
	auto UpdateEquipSlot = [](UItemSlotWidget* TargetSlot, FName ItemID) {
		if (TargetSlot) TargetSlot->SetItemInfo(ItemID, ItemID.IsNone() ? 0 : 1);
		};

	UpdateEquipSlot(WBP_VestSlot, Inv->EquippedVestID);
	UpdateEquipSlot(WBP_HelmetSlot, Inv->EquippedHelmetID);
	UpdateEquipSlot(WBP_BackpackSlot, Inv->EquippedBackpackID);

	// [리팩토링] 무기 장착 슬롯 업데이트 람다 함수 (중복 제거)
	auto UpdateWeaponSlot = [](UItemSlotWidget* TargetSlot, FName ItemID) {
		if (TargetSlot)
		{
			TargetSlot->bIsEquipSlot = true;
			TargetSlot->bIsWeaponSlot = true;
			TargetSlot->SetItemInfo(ItemID, ItemID.IsNone() ? 0 : 1);
		}
		};

	UpdateWeaponSlot(WBP_Weapon1Slot, Inv->EquippedWeapon1ID);
	UpdateWeaponSlot(WBP_Weapon2Slot, Inv->EquippedWeapon2ID);
	UpdateWeaponSlot(WBP_ThrowableSlot, Inv->EquippedThrowableID);

	if (WBP_PistolSlot)
	{
		WBP_PistolSlot->bIsEquipSlot = true;
		WBP_PistolSlot->bIsWeaponSlot = true;
		WBP_PistolSlot->bIsLockedSlot = true;
		WBP_PistolSlot->SetItemInfo(FName("DefaultPistol"), 1);
	}

	// [리팩토링] 부착물 컨테이너 가시성 업데이트 람다 함수
	auto UpdateContainerVisibility = [](UPanelWidget* Container, FName WeaponID) {
		if (Container) Container->SetVisibility(WeaponID.IsNone() ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
		};

	UpdateContainerVisibility(Container_Weapon1_Parts, Inv->EquippedWeapon1ID);
	UpdateContainerVisibility(Container_Weapon2_Parts, Inv->EquippedWeapon2ID);

	// 파츠 슬롯 초기화
	auto ClearAttachment = [&](UItemSlotWidget* TargetSlot) { if (TargetSlot) TargetSlot->SetItemInfo(NAME_None, 0); };
	ClearAttachment(WBP_AttachmentSlot1_Scope);
	ClearAttachment(WBP_AttachmentSlot1_Muzzle);
	ClearAttachment(WBP_AttachmentSlot1_Magazine);
	ClearAttachment(WBP_AttachmentSlot2_Scope);
	ClearAttachment(WBP_AttachmentSlot2_Muzzle);
	ClearAttachment(WBP_AttachmentSlot2_Magazine);
}

bool UInventoryMainWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);

	UItemDragOperation* DragOp = Cast<UItemDragOperation>(InOperation);
	if (!DragOp) return false;

	FVector2D DropPos = InDragDropEvent.GetScreenSpacePosition();
	EDropZoneType TargetZone = EDropZoneType::Nearby;

	auto IsUnder = [&](UWidget* Widget) { return Widget && Widget->GetCachedGeometry().IsUnderLocation(DropPos); };

	if (IsUnder(InventoryScrollBox)) TargetZone = EDropZoneType::Backpack;
	else if (IsUnder(WBP_VestSlot) || IsUnder(WBP_HelmetSlot) || IsUnder(WBP_BackpackSlot) ||
		IsUnder(IMG_CharacterPreview) || IsUnder(WBP_Weapon1Slot) || IsUnder(WBP_Weapon2Slot) || IsUnder(WBP_ThrowableSlot))
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
	EDropZoneType SourceZone = Operation->bIsFromGround ? EDropZoneType::Nearby : (Operation->bIsFromEquip ? EDropZoneType::Equipment : EDropZoneType::Backpack);

	if (SourceZone == TargetZone) return;

	if (CurrentMode == EInventoryMode::IM_Shop)
	{
		if (SourceZone == EDropZoneType::Nearby && TargetZone == EDropZoneType::Backpack)
		{
			if (CurrentShopNPC && CurrentShopNPC->ShopInventory.Contains(ItemID))
			{
				PromptQuantitySelection(ItemID, CurrentShopNPC->ShopInventory[ItemID], true);
			}
		}
		else if (SourceZone == EDropZoneType::Backpack && TargetZone == EDropZoneType::Nearby)
		{
			// [리팩토링] 이전에 만들었던 GetTotalItemCount를 활용하여 for문 제거!
			int32 TotalPlayerStock = Player->PlayerInventory->GetTotalItemCount(ItemID);
			if (TotalPlayerStock > 0)
			{
				PromptQuantitySelection(ItemID, TotalPlayerStock, false);
			}
		}
		return;
	}

	if (TargetZone == EDropZoneType::Equipment)
	{
		FItemData* ItemData = Player->PlayerInventory->ItemDataTable->FindRow<FItemData>(ItemID, TEXT("DropTypeCheck"));
		if (!ItemData || (ItemData->ItemType != EItemType::Equipment && ItemData->ItemType != EItemType::Weapon && ItemData->ItemType != EItemType::Throwable)) return;
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

	Player->RefreshStudioEquipment();
	RefreshInventory();
	ForceNearbyRefresh();
}

void UInventoryMainWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragCancelled(InDragDropEvent, InOperation);
	if (UItemDragOperation* DragOp = Cast<UItemDragOperation>(InOperation))
	{
		HandleItemDrop(DragOp, EDropZoneType::Nearby);
	}
}

void UInventoryMainWidget::OpenShopMode(const TMap<FName, int32>& ShopItems)
{
	CurrentMode = EInventoryMode::IM_Shop;
	StopNearbyTimer();
	UpdateShopUI(ShopItems);
}

void UInventoryMainWidget::UpdateShopUI(const TMap<FName, int32>& ShopItems)
{
	if (!VicinityScrollBox || !ItemSlotWidgetClass) return;

	HideTooltip();
	VicinityScrollBox->ClearChildren();

	for (const TPair<FName, int32>& Pair : ShopItems)
	{
		if (UItemSlotWidget* NewSlot = CreateWidget<UItemSlotWidget>(this, ItemSlotWidgetClass))
		{
			NewSlot->bIsVicinitySlot = true;
			NewSlot->TargetItemActor = nullptr;
			NewSlot->SetItemInfo(Pair.Key, Pair.Value);
			VicinityScrollBox->AddChild(NewSlot);
		}
	}
}

void UInventoryMainWidget::BuyItem(FName ItemID)
{
	AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwningPlayerPawn());
	FItemData* ItemData = Player->PlayerInventory->ItemDataTable->FindRow<FItemData>(ItemID, TEXT("BuyCheck"));
	if (!ItemData || !CurrentShopNPC) return;

	if (Player->SpendGold(ItemData->BuyPrice))
	{
		int32 UnaddedAmount = Player->PlayerInventory->AddItem(ItemID, 1);
		if (UnaddedAmount > 0)
		{
			Player->AddGold(ItemData->BuyPrice);
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("Full Backpack!"));
		}
	}
	else
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("Need More Gold!"));
	}

	RefreshInventory();
	UpdateShopUI(CurrentShopNPC->ShopInventory);
}

void UInventoryMainWidget::SellItem(FName ItemID)
{
	AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwningPlayerPawn());
	FItemData* ItemData = Player->PlayerInventory->ItemDataTable->FindRow<FItemData>(ItemID, TEXT("SellCheck"));
	if (!ItemData || !CurrentShopNPC) return;

	if (Player->PlayerInventory->RemoveItem(ItemID, 1))
	{
		Player->AddGold(ItemData->SellPrice);
	}

	RefreshInventory();
	UpdateShopUI(CurrentShopNPC->ShopInventory);
}

void UInventoryMainWidget::StartNearbyTimer()
{
	if (!GetWorld()->GetTimerManager().IsTimerActive(NearbyCheckTimer))
	{
		GetWorld()->GetTimerManager().SetTimer(NearbyCheckTimer, this, &UInventoryMainWidget::CheckNearbyItems, 0.2f, true);
	}
}

void UInventoryMainWidget::StopNearbyTimer()
{
	GetWorld()->GetTimerManager().ClearTimer(NearbyCheckTimer);
}

void UInventoryMainWidget::ConfirmBuyItem(FName ItemID, int32 AmountToBuy)
{
	AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwningPlayerPawn());
	if (!Player || !CurrentShopNPC || !CurrentShopNPC->ShopInventory.Contains(ItemID)) return;

	FItemData* ItemData = Player->PlayerInventory->ItemDataTable->FindRow<FItemData>(ItemID, TEXT("BuyCheck"));
	if (!ItemData || AmountToBuy <= 0 || AmountToBuy > CurrentShopNPC->ShopInventory[ItemID]) return;

	if (Player->SpendGold(ItemData->BuyPrice * AmountToBuy))
	{
		int32 UnaddedAmount = Player->PlayerInventory->AddItem(ItemID, AmountToBuy);
		int32 SuccessfullyAdded = AmountToBuy - UnaddedAmount;

		if (SuccessfullyAdded > 0)
		{
			CurrentShopNPC->ShopInventory[ItemID] -= SuccessfullyAdded;
			if (CurrentShopNPC->ShopInventory[ItemID] <= 0) CurrentShopNPC->ShopInventory.Remove(ItemID);
		}

		if (UnaddedAmount > 0) Player->AddGold(UnaddedAmount * ItemData->BuyPrice);

		RefreshInventory();
		UpdateShopUI(CurrentShopNPC->ShopInventory);
	}
}

void UInventoryMainWidget::ConfirmSellItem(FName ItemID, int32 AmountToSell)
{
	AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwningPlayerPawn());
	if (!Player || AmountToSell <= 0 || !CurrentShopNPC) return;

	FItemData* ItemData = Player->PlayerInventory->ItemDataTable->FindRow<FItemData>(ItemID, TEXT("SellCheck"));
	if (!ItemData) return;

	if (Player->PlayerInventory->RemoveItem(ItemID, AmountToSell))
	{
		Player->AddGold(ItemData->SellPrice * AmountToSell);
		RefreshInventory();
		UpdateShopUI(CurrentShopNPC->ShopInventory);
	}
}

void UInventoryMainWidget::PromptQuantitySelection(FName ItemID, int32 MaxAvailable, bool bIsBuying)
{
	if (!QuantityPopupClass) return;

	AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwningPlayerPawn());
	if (!Player || !Player->PlayerInventory || !Player->PlayerInventory->ItemDataTable) return;

	FItemData* ItemData = Player->PlayerInventory->ItemDataTable->FindRow<FItemData>(ItemID, TEXT("PopupPriceCheck"));
	if (!ItemData) return;

	int32 TargetUnitPrice = bIsBuying ? ItemData->BuyPrice : ItemData->SellPrice;
	int32 AffordableQty = (bIsBuying && TargetUnitPrice > 0) ? (Player->CurrentGold / TargetUnitPrice) : MaxAvailable;

	if (UQuantityPopupWidget* Popup = CreateWidget<UQuantityPopupWidget>(this, QuantityPopupClass))
	{
		Popup->SetupPopup(ItemID, MaxAvailable, AffordableQty, TargetUnitPrice, bIsBuying, this);
		Popup->AddToViewport();
		Popup->SetKeyboardFocus();
	}
}

void UInventoryMainWidget::UpdateCurrencyUI()
{
	if (AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwningPlayerPawn()))
	{
		if (Txt_GoldAmount) Txt_GoldAmount->SetText(FText::AsNumber(Player->CurrentGold));
		if (Txt_SapphireAmount) Txt_SapphireAmount->SetText(FText::AsNumber(Player->CurrentSapphire));
	}
}