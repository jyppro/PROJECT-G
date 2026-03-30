#include "ItemSlotWidget.h"
#include "Components/Image.h"      // UImage 제어용
#include "Components/TextBlock.h"  // UTextBlock 제어용
#include "Engine/Texture2D.h"
#include "../Gun_phiriaCharacter.h"
#include "../component/InventoryComponent.h"

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

FReply UItemSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		if (AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwningPlayerPawn()))
		{
			if (UInventoryComponent* Inventory = Player->PlayerInventory)
			{
				Inventory->UseItemByID(CurrentItemID);
				return FReply::Handled();
			}
		}
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}