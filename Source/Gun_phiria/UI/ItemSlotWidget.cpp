#include "ItemSlotWidget.h"
#include "Components/Image.h"      // UImage 제어용
#include "Components/TextBlock.h"  // UTextBlock 제어용
#include "Engine/Texture2D.h"
#include "../Gun_phiriaCharacter.h"
#include "../component/InventoryComponent.h"
#include "InventoryMainWidget.h"
#include "Components/PanelWidget.h"
#include "../Item/PickupItemBase.h"

// FItemData 구조체가 정의된 헤더를 반드시 포함해야 합니다! 
// (본인의 프로젝트 경로에 맞게 수정해 주세요. 예: #include "Item/ItemData.h")
// 만약 InventoryComponent.h 안에 FItemData가 있다면 이 주석은 무시하셔도 됩니다.

void UItemSlotWidget::SetItemInfo(FName InItemID, int32 InQuantity)
{
	// 1. 아이템 ID 저장 (우클릭 사용을 위해)
	CurrentItemID = InItemID;

	// 2. 수량 텍스트 업데이트 (숫자를 텍스트로 변환)
	if (TXT_ItemQuantity)
	{
		TXT_ItemQuantity->SetText(FText::AsNumber(InQuantity));
	}

	// 3. 데이터 테이블이 에디터에서 잘 연결되었는지 확인
	if (!ItemDataTable)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("ItemSlotWidget에 데이터 테이블(ItemDataTable)이 설정되지 않았습니다!"));
		return;
	}

	// 4. 데이터 테이블에서 아이템 정보 검색
	static const FString ContextString(TEXT("Item Slot Lookup"));
	FItemData* ItemInfo = ItemDataTable->FindRow<FItemData>(InItemID, ContextString);

	// 5. 아이템을 찾았다면 UI에 정보 적용
	if (ItemInfo)
	{
		// 아이콘 이미지 변경
		if (IMG_ItemIcon && ItemInfo->ItemIcon)
		{
			IMG_ItemIcon->SetBrushFromTexture(ItemInfo->ItemIcon);
		}

		// 아이템 이름 변경
		if (TXT_ItemName)
		{
			// 만약 FItemData의 ItemName이 FString이면 FText::FromString(ItemInfo->ItemName)을 사용하세요.
			// FText로 선언되어 있다면 그대로 ItemInfo->ItemName 을 사용하시면 됩니다.
			TXT_ItemName->SetText(ItemInfo->ItemName);
		}
	}
	else
	{
		// 아이템을 찾지 못했을 때의 예외 처리 (디버그용)
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("데이터 테이블에서 아이템을 찾을 수 없습니다: %s"), *InItemID.ToString()));
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

	if (UInventoryMainWidget* MainUI = GetTypedOuter<UInventoryMainWidget>())
	{
		MainUI->ShowTooltip(CurrentItemID, ItemDataTable);
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
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		if (AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwningPlayerPawn()))
		{
			if (bIsVicinitySlot)
			{
				// [바닥 슬롯] 우클릭 -> 인벤토리에 넣기 (습득)
				int32 Remainder = Player->PlayerInventory->AddItem(CurrentItemID, 1);

				if (Remainder == 0) // 가방 공간이 있어서 성공했다면
				{
					if (TargetItemActor && IsValid(TargetItemActor))
					{
						TargetItemActor->Destroy();
						TargetItemActor = nullptr;
					}

					if (UInventoryMainWidget* MainUI = GetTypedOuter<UInventoryMainWidget>())
					{
						// (지웠던 HideTooltip의 빈자리. 이제 MainUI가 알아서 꺼줍니다!)
						MainUI->RefreshInventory();
						MainUI->ForceNearbyRefresh();
					}
				}
			}
			else
			{
				// [가방 슬롯] 우클릭 -> 아이템 사용
				// (여기도 HideTooltip 삭제 완료!)
				Player->PlayerInventory->UseItemByID(CurrentItemID);
			}
			return FReply::Handled();
		}
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}