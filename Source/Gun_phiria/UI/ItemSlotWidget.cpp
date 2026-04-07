#include "ItemSlotWidget.h"
#include "Components/Image.h"      // UImage 제어용
#include "Components/TextBlock.h"  // UTextBlock 제어용
#include "Engine/Texture2D.h"
#include "../Gun_phiriaCharacter.h"
#include "../component/InventoryComponent.h"
#include "InventoryMainWidget.h"
#include "Components/PanelWidget.h"
#include "ItemDragOperation.h"
#include "../Item/PickupItemBase.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "DragVisualWidget.h"

// FItemData 구조체가 정의된 헤더를 반드시 포함해야 합니다! 
// (본인의 프로젝트 경로에 맞게 수정해 주세요. 예: #include "Item/ItemData.h")
// 만약 InventoryComponent.h 안에 FItemData가 있다면 이 주석은 무시하셔도 됩니다.

void UItemSlotWidget::SetItemInfo(FName InItemID, int32 InQuantity)
{
	CurrentItemID = InItemID;
	CurrentQuantity = InQuantity;

	// 아이템이 없으면 UI 숨기기
	if (CurrentItemID.IsNone() || CurrentQuantity <= 0)
	{
		if (IMG_ItemIcon) IMG_ItemIcon->SetVisibility(ESlateVisibility::Hidden);
		if (TXT_ItemName) TXT_ItemName->SetVisibility(ESlateVisibility::Hidden);
		if (TXT_ItemQuantity) TXT_ItemQuantity->SetVisibility(ESlateVisibility::Hidden);

		if (UInventoryMainWidget* MainUI = GetTypedOuter<UInventoryMainWidget>())
		{
			MainUI->HideTooltip();
		}

		return;
	}

	// 데이터 테이블 기반 UI 갱신
	if (ItemDataTable)
	{
		FItemData* ItemInfo = ItemDataTable->FindRow<FItemData>(CurrentItemID, TEXT("SlotUpdate"));
		if (ItemInfo)
		{
			if (IMG_ItemIcon && ItemInfo->ItemIcon)
			{
				IMG_ItemIcon->SetBrushFromTexture(ItemInfo->ItemIcon);
				IMG_ItemIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			}

			if (TXT_ItemName)
			{
				TXT_ItemName->SetText(ItemInfo->ItemName);
				TXT_ItemName->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			}
		}
	}

	// 수량 표시
	if (TXT_ItemQuantity)
	{
		TXT_ItemQuantity->SetText(FText::AsNumber(CurrentQuantity));
		// 1개보다 많을 때만 수량 텍스트를 보여줌
		TXT_ItemQuantity->SetVisibility(CurrentQuantity > 1 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
	}
}

void UItemSlotWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	// 배경색 어둡게 만들기 (호버 효과)
	if (IMG_Background)
	{
		IMG_Background->SetColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f, 0.5f));
	}

	if (!CurrentItemID.IsNone())
	{
		if (UInventoryMainWidget* MainUI = GetTypedOuter<UInventoryMainWidget>())
		{
			MainUI->ShowTooltip(CurrentItemID, ItemDataTable);
		}
	}
}

// 2. 마우스가 나갔을 때 (원래 색으로 복구 + 툴팁 끄기)
void UItemSlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	// 배경색 원래대로 (흰색)
	if (IMG_Background)
	{
		IMG_Background->SetColorAndOpacity(FLinearColor(0.25f, 0.25f, 0.25f, 0.5f));
	}

	if (UInventoryMainWidget* MainUI = GetTypedOuter<UInventoryMainWidget>())
	{
		MainUI->HideTooltip();
	}
}

FReply UItemSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (CurrentItemID.IsNone()) return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);

	if (bIsLockedSlot)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	// [1] 우클릭 (빠른 이동 및 장착/사용 로직)
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwningPlayerPawn());
		if (Player && Player->PlayerInventory)
		{
			if (bIsVicinitySlot)
			{
				// 바닥 -> 가방 (줍기)
				Player->PlayerInventory->AddItem(CurrentItemID, CurrentQuantity);
				if (TargetItemActor) TargetItemActor->Destroy();
			}
			else if (bIsEquipSlot || bIsWeaponSlot || bIsAttachmentSlot)
			{
				// (팁: 나중에 인벤토리 컴포넌트 쪽에 UnequipWeapon 같은 전용 함수를 만들어서
				// if(bIsWeaponSlot) { Player->PlayerInventory->UnequipWeapon(...) } 
				// 이렇게 세분화해주면 훨씬 깔끔합니다. 우선은 기존 함수로 연결!)
				Player->PlayerInventory->UnequipItemByID(CurrentItemID);
			}
			else
			{
				Player->PlayerInventory->UseItemByID(CurrentItemID);
			}

			if (UInventoryMainWidget* MainUI = GetTypedOuter<UInventoryMainWidget>())
			{
				MainUI->RefreshInventory();
				MainUI->ForceNearbyRefresh();
			}
		}
		return FReply::Handled();
	}

	// [2] 좌클릭 (드래그 감지 시작 - Detect Drag If Pressed 대체)
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UItemSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	if (CurrentItemID.IsNone() || bIsLockedSlot) return;

	UItemDragOperation* DragOp = NewObject<UItemDragOperation>(this);
	if (DragOp)
	{
		DragOp->DraggedItemID = CurrentItemID;
		DragOp->bIsFromGround = bIsVicinitySlot;
		DragOp->bIsFromEquip = (bIsEquipSlot || bIsWeaponSlot || bIsAttachmentSlot);
		DragOp->DraggedActor = TargetItemActor;

		AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwningPlayerPawn());
		if (Player && Player->PlayerInventory && DragVisualClass)
		{
			UDragVisualWidget* DragVisual = CreateWidget<UDragVisualWidget>(GetOwningPlayer(), DragVisualClass);

			if (DragVisual)
			{
				// 안전하게 PlayerInventory의 데이터 테이블을 사용
				FItemData* ItemInfo = Player->PlayerInventory->ItemDataTable->FindRow<FItemData>(CurrentItemID, TEXT("DragVisual Setup"));
				if (ItemInfo && ItemInfo->ItemIcon)
				{
					DragVisual->SetDragIcon(ItemInfo->ItemIcon);
				}

				// ========================================================
				// [핵심 해결책] 엔진이 위젯 크기(100x100)를 즉시 계산하도록 강제!
				// (이 한 줄이 0,0에서 날아오는 버그와 멈춤 버그를 모두 고칩니다)
				// ========================================================
				DragVisual->ForceLayoutPrepass();

				DragOp->DefaultDragVisual = DragVisual;
				DragOp->Pivot = EDragPivot::CenterCenter; // CenterCenter로 복구
			}
		}

		OutOperation = DragOp;
	}
}

void UItemSlotWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragCancelled(InDragDropEvent, InOperation);

	UItemDragOperation* DragOp = Cast<UItemDragOperation>(InOperation);
	UInventoryMainWidget* MainUI = GetTypedOuter<UInventoryMainWidget>();

	if (DragOp && MainUI)
	{
		// 드래그가 취소되었다 = 바닥 영역에 마우스를 놨다!
		MainUI->HandleItemDrop(DragOp, EDropZoneType::Nearby);
	}
}