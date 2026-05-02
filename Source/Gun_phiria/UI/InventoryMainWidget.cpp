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

	// [수정] 상점 모드(거래 또는 약탈)일 때의 드래그 처리
	if (CurrentMode == EInventoryMode::IM_Shop)
	{
		// 1. 상점 -> 가방 (가져오기)
		if (SourceZone == EDropZoneType::Nearby && TargetZone == EDropZoneType::Backpack)
		{
			if (CurrentShopNPC && CurrentShopNPC->ShopInventory.Contains(ItemID))
			{
				int32 AvailableQty = CurrentShopNPC->ShopInventory[ItemID];
				// 약탈이든 거래든 무조건 팝업을 띄웁니다!
				PromptQuantitySelection(ItemID, AvailableQty, true);
			}
		}
		// 2. 가방 -> 상점 (넣기/팔기)
		else if (SourceZone == EDropZoneType::Backpack && TargetZone == EDropZoneType::Nearby)
		{
			int32 TotalPlayerStock = Player->PlayerInventory->GetTotalItemCount(ItemID);
			if (TotalPlayerStock > 0)
			{
				PromptQuantitySelection(ItemID, TotalPlayerStock, false);
			}
		}
		return;
	}

	// ... (아래쪽 일반 인벤토리 동작 코드는 기존과 동일하게 유지) ...
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

void UInventoryMainWidget::BuyItem(FName ItemID) // 마우스 우클릭 빠른 구매
{
	AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwningPlayerPawn());
	if (!Player || !CurrentShopNPC || !CurrentShopNPC->ShopInventory.Contains(ItemID)) return;

	FItemData* ItemData = Player->PlayerInventory->ItemDataTable->FindRow<FItemData>(ItemID, TEXT("BuyCheck"));
	if (!ItemData) return;

	// [수정] 약탈 모드면 돈 지불 패스
	bool bCanAfford = bIsShopLootMode ? true : Player->SpendGold(ItemData->BuyPrice);

	if (bCanAfford)
	{
		int32 UnaddedAmount = Player->PlayerInventory->AddItem(ItemID, 1);
		if (UnaddedAmount == 0) // 무사히 들어갔을 때만 상점에서 삭제!
		{
			CurrentShopNPC->ShopInventory[ItemID] -= 1;
			if (CurrentShopNPC->ShopInventory[ItemID] <= 0) CurrentShopNPC->ShopInventory.Remove(ItemID);
		}
		else // 가방이 꽉 참
		{
			if (!bIsShopLootMode) Player->AddGold(ItemData->BuyPrice); // 돈 환불
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

void UInventoryMainWidget::SellItem(FName ItemID) // 마우스 우클릭 빠른 판매
{
	AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwningPlayerPawn());
	if (!Player || !CurrentShopNPC) return;

	FItemData* ItemData = Player->PlayerInventory->ItemDataTable->FindRow<FItemData>(ItemID, TEXT("SellCheck"));
	if (!ItemData) return;

	if (Player->PlayerInventory->RemoveItem(ItemID, 1))
	{
		if (bIsShopLootMode)
		{
			// [수정] 시체에 넣기
			if (CurrentShopNPC->ShopInventory.Contains(ItemID)) CurrentShopNPC->ShopInventory[ItemID] += 1;
			else CurrentShopNPC->ShopInventory.Add(ItemID, 1);
		}
		else
		{
			Player->AddGold(ItemData->SellPrice);
		}
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

	// [수정] 약탈 모드면 돈 지불 패스!
	bool bCanAfford = bIsShopLootMode ? true : Player->SpendGold(ItemData->BuyPrice * AmountToBuy);

	if (bCanAfford)
	{
		// 가방에 들어간 만큼만 상점에서 차감합니다. (복사 버그 원천 차단)
		int32 UnaddedAmount = Player->PlayerInventory->AddItem(ItemID, AmountToBuy);
		int32 SuccessfullyAdded = AmountToBuy - UnaddedAmount;

		if (SuccessfullyAdded > 0)
		{
			CurrentShopNPC->ShopInventory[ItemID] -= SuccessfullyAdded;
			if (CurrentShopNPC->ShopInventory[ItemID] <= 0) CurrentShopNPC->ShopInventory.Remove(ItemID);
		}

		// 다 들어가지 못했고 정상 거래였다면 돈을 돌려줍니다.
		if (UnaddedAmount > 0)
		{
			if (!bIsShopLootMode) Player->AddGold(UnaddedAmount * ItemData->BuyPrice);
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("Not enough space!"));
		}

		RefreshInventory();
		UpdateShopUI(CurrentShopNPC->ShopInventory);
	}
	else
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("Need More Gold!"));
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
		if (bIsShopLootMode)
		{
			// [수정] 약탈 모드: 시체에 아이템을 돌려놓고 돈은 안 받음
			if (CurrentShopNPC->ShopInventory.Contains(ItemID)) CurrentShopNPC->ShopInventory[ItemID] += AmountToSell;
			else CurrentShopNPC->ShopInventory.Add(ItemID, AmountToSell);
		}
		else
		{
			// 정상 판매
			Player->AddGold(ItemData->SellPrice * AmountToSell);
		}

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

	// [핵심] 약탈 모드라면 단가(TargetUnitPrice)를 0으로 만듭니다!
	int32 TargetUnitPrice = bIsShopLootMode ? 0 : (bIsBuying ? ItemData->BuyPrice : ItemData->SellPrice);

	// 단가가 0보다 크면 돈에 맞춰 최대 구매 가능 수량을 계산하고, 0(약탈/무료)이면 가진 수량 전부를 선택 가능하게 합니다.
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