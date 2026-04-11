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
#include "../NPC/ShopNPC.h"
#include "QuantityPopupWidget.h"
#include "Components/TextBlock.h"

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

	// ==========================================
	// --- [1] 주무기 1 (Primary Weapon) ---
	// ==========================================
	if (WBP_Weapon1Slot)
	{
		WBP_Weapon1Slot->bIsEquipSlot = true;
		WBP_Weapon1Slot->bIsWeaponSlot = true;
		WBP_Weapon1Slot->SetItemInfo(Inventory->EquippedWeapon1ID, 1);
	}

	// 파츠 슬롯들도 빈칸(NAME_None)으로 초기화 (나중에 부착물 데이터가 추가되면 여기에 ID를 넣습니다)
	if (WBP_AttachmentSlot1_Scope) WBP_AttachmentSlot1_Scope->SetItemInfo(NAME_None, 0);
	if (WBP_AttachmentSlot1_Muzzle) WBP_AttachmentSlot1_Muzzle->SetItemInfo(NAME_None, 0);
	if (WBP_AttachmentSlot1_Magazine) WBP_AttachmentSlot1_Magazine->SetItemInfo(NAME_None, 0);

	// ==========================================
	// --- [2] 주무기 2 (Secondary Weapon) ---
	// ==========================================
	if (WBP_Weapon2Slot)
	{
		WBP_Weapon2Slot->bIsEquipSlot = true;
		WBP_Weapon2Slot->bIsWeaponSlot = true;
		WBP_Weapon2Slot->SetItemInfo(Inventory->EquippedWeapon2ID, 1);
	}

	if (WBP_AttachmentSlot2_Scope) WBP_AttachmentSlot2_Scope->SetItemInfo(NAME_None, 0);
	if (WBP_AttachmentSlot2_Muzzle) WBP_AttachmentSlot2_Muzzle->SetItemInfo(NAME_None, 0);
	if (WBP_AttachmentSlot2_Magazine) WBP_AttachmentSlot2_Magazine->SetItemInfo(NAME_None, 0);

	// ==========================================
	// --- [3] 권총 (Pistol) ---
	// ==========================================
	if (WBP_PistolSlot)
	{
		WBP_PistolSlot->bIsEquipSlot = true;
		WBP_PistolSlot->bIsWeaponSlot = true;

		// 블루프린트에서 권총을 드래그 못하게 체크했다면 코드에서도 확인
		WBP_PistolSlot->bIsLockedSlot = true;

		// 인벤토리 변수를 읽지 않고, 데이터 테이블의 "DefaultPistol"을 강제로 고정!
		WBP_PistolSlot->SetItemInfo(FName("DefaultPistol"), 1);
	}

	// ==========================================
	// --- [4] 투척 무기 (Throwable) ---
	// ==========================================
	if (WBP_ThrowableSlot)
	{
		WBP_ThrowableSlot->bIsEquipSlot = true;
		WBP_ThrowableSlot->bIsWeaponSlot = true;
		WBP_ThrowableSlot->SetItemInfo(Inventory->EquippedThrowableID, 1);
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
	else if ((WBP_VestSlot && WBP_VestSlot->GetCachedGeometry().IsUnderLocation(DropPosition)) ||
		(WBP_HelmetSlot && WBP_HelmetSlot->GetCachedGeometry().IsUnderLocation(DropPosition)) ||
		(WBP_BackpackSlot && WBP_BackpackSlot->GetCachedGeometry().IsUnderLocation(DropPosition)) ||
		(IMG_CharacterPreview && IMG_CharacterPreview->GetCachedGeometry().IsUnderLocation(DropPosition)) ||
		(WBP_Weapon1Slot && WBP_Weapon1Slot->GetCachedGeometry().IsUnderLocation(DropPosition)) ||
		(WBP_Weapon2Slot && WBP_Weapon2Slot->GetCachedGeometry().IsUnderLocation(DropPosition)) ||
		(WBP_ThrowableSlot && WBP_ThrowableSlot->GetCachedGeometry().IsUnderLocation(DropPosition)))
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

	if (CurrentMode == EInventoryMode::IM_Shop)
	{
		// 1. 상점 -> 가방 (구매)
		if (SourceZone == EDropZoneType::Nearby && TargetZone == EDropZoneType::Backpack)
		{
			if (CurrentShopNPC && CurrentShopNPC->ShopInventory.Contains(ItemID))
			{
				int32 MaxNPCStock = CurrentShopNPC->ShopInventory[ItemID];
				PromptQuantitySelection(ItemID, MaxNPCStock, true); // true = 구매
			}
		}
		// 2. 가방 -> 상점 (판매)
		else if (SourceZone == EDropZoneType::Backpack && TargetZone == EDropZoneType::Nearby)
		{
			// 플레이어 인벤토리를 뒤져서 이 아이템을 총 몇 개 가졌는지 확인
			int32 TotalPlayerStock = 0;
			for (const FInventorySlot& InvSlot : Player->PlayerInventory->InventorySlots)
			{
				if (InvSlot.ItemID == ItemID) TotalPlayerStock += InvSlot.Quantity;
			}

			if (TotalPlayerStock > 0)
			{
				PromptQuantitySelection(ItemID, TotalPlayerStock, false); // false = 판매
			}
		}

		return; // 일반 파밍 로직 실행 방지
	}

	if (TargetZone == EDropZoneType::Equipment)
	{
		FItemData* ItemData = Player->PlayerInventory->ItemDataTable->FindRow<FItemData>(ItemID, TEXT("DropTypeCheck"));

		if (!ItemData || (ItemData->ItemType != EItemType::Equipment &&
			ItemData->ItemType != EItemType::Weapon &&
			ItemData->ItemType != EItemType::Throwable))
		{
			return; // 방어구, 무기, 투척물이 아니면 (예: 소비 아이템 등) 장착 불가
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

void UInventoryMainWidget::OpenShopMode(const TMap<FName, int32>& ShopItems)
{
	CurrentMode = EInventoryMode::IM_Shop;
	GetWorld()->GetTimerManager().ClearTimer(NearbyCheckTimer);
	UpdateShopUI(ShopItems);
}

void UInventoryMainWidget::UpdateShopUI(const TMap<FName, int32>& ShopItems)
{
	if (!VicinityScrollBox || !ItemSlotWidgetClass) return;

	HideTooltip();
	VicinityScrollBox->ClearChildren();

	// TMap 순회 (Pair.Key가 ItemID, Pair.Value가 Quantity)
	for (const TPair<FName, int32>& Pair : ShopItems)
	{
		UItemSlotWidget* NewSlot = CreateWidget<UItemSlotWidget>(this, ItemSlotWidgetClass);
		if (NewSlot)
		{
			NewSlot->bIsVicinitySlot = true;
			NewSlot->TargetItemActor = nullptr;
			NewSlot->SetItemInfo(Pair.Key, Pair.Value); // NPC가 가진 수량 띄우기
			VicinityScrollBox->AddChild(NewSlot);
		}
	}
}

void UInventoryMainWidget::BuyItem(FName ItemID)
{
	AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwningPlayerPawn());
	FItemData* ItemData = Player->PlayerInventory->ItemDataTable->FindRow<FItemData>(ItemID, TEXT("BuyCheck"));
	if (!ItemData) return;

	// 돈이 충분한지 확인
	if (Player->SpendGold(ItemData->BuyPrice))
	{
		// 아이템 추가 시도 (무게 체크 등은 네가 만든 AddItem이 다 해줌!)
		int32 UnaddedAmount = Player->PlayerInventory->AddItem(ItemID, 1);

		if (UnaddedAmount > 0)
		{
			// 가방 공간이 없어서 못 샀다면 돈을 환불해 줍니다.
			Player->AddGold(ItemData->BuyPrice);
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("Full Backpack!"));
		}
		else
		{
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, FString::Printf(TEXT("%s Complete Buy!"), *ItemData->ItemName.ToString()));
		}
	}
	else
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("Need More Gold!"));
	}

	RefreshInventory(); // 이 안에 UpdateCurrencyUI가 들어있으므로 자동으로 갱신됨!
	UpdateShopUI(CurrentShopNPC->ShopInventory);
}

void UInventoryMainWidget::SellItem(FName ItemID)
{
	AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwningPlayerPawn());
	FItemData* ItemData = Player->PlayerInventory->ItemDataTable->FindRow<FItemData>(ItemID, TEXT("SellCheck"));
	if (!ItemData) return;

	// 인벤토리에서 아이템 1개 제거 (네가 만든 RemoveItem 활용)
	if (Player->PlayerInventory->RemoveItem(ItemID, 1))
	{
		// 골드 지급
		Player->AddGold(ItemData->SellPrice);
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, FString::Printf(TEXT("%s Complete Sell! +%d G"), *ItemData->ItemName.ToString(), ItemData->SellPrice));
	}

	RefreshInventory(); // 이 안에 UpdateCurrencyUI가 들어있으므로 자동으로 갱신됨!
	UpdateShopUI(CurrentShopNPC->ShopInventory);
}

void UInventoryMainWidget::StartNearbyTimer()
{
	// 타이머가 이미 돌고 있지 않을 때만 새로 시작
	if (!GetWorld()->GetTimerManager().IsTimerActive(NearbyCheckTimer))
	{
		GetWorld()->GetTimerManager().SetTimer(NearbyCheckTimer, this, &UInventoryMainWidget::CheckNearbyItems, 0.2f, true);
	}
}

void UInventoryMainWidget::StopNearbyTimer()
{
	// 타이머 정지
	GetWorld()->GetTimerManager().ClearTimer(NearbyCheckTimer);
}

void UInventoryMainWidget::ConfirmBuyItem(FName ItemID, int32 AmountToBuy)
{
	AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwningPlayerPawn());
	if (!Player || !CurrentShopNPC || !CurrentShopNPC->ShopInventory.Contains(ItemID)) return;

	FItemData* ItemData = Player->PlayerInventory->ItemDataTable->FindRow<FItemData>(ItemID, TEXT("BuyCheck"));
	if (!ItemData || AmountToBuy <= 0) return;

	// 내가 사려는 개수만큼의 총 가격 계산
	int32 TotalCost = ItemData->BuyPrice * AmountToBuy;

	// NPC 재고 체크 방어 코드
	if (AmountToBuy > CurrentShopNPC->ShopInventory[ItemID]) return;

	if (Player->SpendGold(TotalCost))
	{
		// 인벤토리에 아이템 추가 시도
		int32 UnaddedAmount = Player->PlayerInventory->AddItem(ItemID, AmountToBuy);
		int32 SuccessfullyAdded = AmountToBuy - UnaddedAmount;

		if (SuccessfullyAdded > 0)
		{
			// 성공적으로 가방에 넣은 개수만큼만 NPC 재고에서 차감
			CurrentShopNPC->ShopInventory[ItemID] -= SuccessfullyAdded;

			// NPC 재고가 0이 되면 리스트에서 삭제
			if (CurrentShopNPC->ShopInventory[ItemID] <= 0)
			{
				CurrentShopNPC->ShopInventory.Remove(ItemID);
			}

			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, FString::Printf(TEXT("%s %d Buy Complete!"), *ItemData->ItemName.ToString(), SuccessfullyAdded));
		}

		// 가방 공간 부족으로 못 산 아이템이 있다면 골드 환불
		if (UnaddedAmount > 0)
		{
			Player->AddGold(UnaddedAmount * ItemData->BuyPrice);
			//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("가방 용량이 부족하여 일부만 구매했습니다!"));
		}

		// UI들 새로고침
		RefreshInventory();
		UpdateShopUI(CurrentShopNPC->ShopInventory); // NPC 쪽 리스트 새로고침
	}
	else
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("Need More Gold!"));
	}
}

void UInventoryMainWidget::ConfirmSellItem(FName ItemID, int32 AmountToSell)
{
	AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwningPlayerPawn());
	if (!Player || AmountToSell <= 0) return;

	FItemData* ItemData = Player->PlayerInventory->ItemDataTable->FindRow<FItemData>(ItemID, TEXT("SellCheck"));
	if (!ItemData) return;

	// 인벤토리에서 지정한 개수(AmountToSell)만큼 아이템 제거
	if (Player->PlayerInventory->RemoveItem(ItemID, AmountToSell))
	{
		// 개수만큼 판매 금액 정산
		int32 TotalEarned = ItemData->SellPrice * AmountToSell;
		Player->AddGold(TotalEarned);

		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, FString::Printf(TEXT("%s %d Sell Complete! +%d G"), *ItemData->ItemName.ToString(), AmountToSell, TotalEarned));

		// 화면 갱신
		RefreshInventory(); // 이 안에 UpdateCurrencyUI가 들어있으므로 자동으로 갱신됨!
		UpdateShopUI(CurrentShopNPC->ShopInventory);
	}
}

void UInventoryMainWidget::PromptQuantitySelection(FName ItemID, int32 MaxAvailable, bool bIsBuying)
{
	if (QuantityPopupClass)
	{
		AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwningPlayerPawn());
		if (!Player || !Player->PlayerInventory || !Player->PlayerInventory->ItemDataTable) return;

		FItemData* ItemData = Player->PlayerInventory->ItemDataTable->FindRow<FItemData>(ItemID, TEXT("PopupPriceCheck"));
		if (!ItemData) return;

		int32 TargetUnitPrice = bIsBuying ? ItemData->BuyPrice : ItemData->SellPrice;

		// ==========================================================
		// [추가된 부분] 내 돈으로 살 수 있는 개수 계산!
		// ==========================================================
		int32 AffordableQty = MaxAvailable; // 기본은 상점 재고만큼

		if (bIsBuying && TargetUnitPrice > 0)
		{
			// 현재 골드 나누기 가격 = 살 수 있는 개수
			// (주의: Player->CurrentGold 변수가 protected로 되어있어 에러가 난다면,
			// 캐릭터 헤더에서 public으로 바꾸거나 GetGold() 같은 함수를 만들어야 해!)
			AffordableQty = Player->CurrentGold / TargetUnitPrice;
		}

		UQuantityPopupWidget* Popup = CreateWidget<UQuantityPopupWidget>(this, QuantityPopupClass);
		if (Popup)
		{
			// 변경된 SetupPopup! AffordableQty를 넘겨줍니다.
			Popup->SetupPopup(ItemID, MaxAvailable, AffordableQty, TargetUnitPrice, bIsBuying, this);
			Popup->AddToViewport();

			Popup->SetKeyboardFocus();
		}
	}
}

void UInventoryMainWidget::UpdateCurrencyUI()
{
	AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwningPlayerPawn());
	if (!Player) return;

	// 골드 텍스트 갱신
	if (Txt_GoldAmount)
	{
		// Player->CurrentGold 변수에 접근할 수 있다고 가정합니다.
		Txt_GoldAmount->SetText(FText::AsNumber(Player->CurrentGold));
	}

	// 사파이어 텍스트 갱신
	if (Txt_SapphireAmount)
	{
		Txt_SapphireAmount->SetText(FText::AsNumber(Player->CurrentSapphire));
	}
}