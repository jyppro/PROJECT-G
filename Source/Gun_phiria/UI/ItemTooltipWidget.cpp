#include "ItemTooltipWidget.h"
#include "Components/TextBlock.h"
#include "Engine/DataTable.h"
#include "../component/InventoryComponent.h" // FItemData 구조체 위치
#include "Components/Image.h"

void UItemTooltipWidget::UpdateTooltip(FName ItemID, UDataTable* ItemDataTable)
{
	// 1. 데이터 테이블이 없거나 아이템 ID가 비어있다면(파괴됨/빈칸) 툴팁을 즉시 숨깁니다!
	if (!ItemDataTable || ItemID.IsNone())
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	static const FString ContextString(TEXT("Tooltip Lookup"));
	FItemData* ItemInfo = ItemDataTable->FindRow<FItemData>(ItemID, ContextString);

	// 2. 아이템 정보가 존재할 때
	if (ItemInfo)
	{
		// 툴팁을 다시 화면에 보이게 만듭니다. (마우스 클릭을 방해하지 않게 HitTestInvisible 사용)
		SetVisibility(ESlateVisibility::HitTestInvisible);

		if (TXT_Name) TXT_Name->SetText(ItemInfo->ItemName);
		if (TXT_Description) TXT_Description->SetText(ItemInfo->ItemDescription);

		if (IMG_ItemIcon && ItemInfo->ItemIcon)
		{
			IMG_ItemIcon->SetBrushFromTexture(ItemInfo->ItemIcon);
		}

		if (TXT_Type)
		{
			const UEnum* EnumPtr = StaticEnum<EItemType>();
			if (EnumPtr)
			{
				FText TypeText = EnumPtr->GetDisplayNameTextByValue(static_cast<int64>(ItemInfo->ItemType));
				TXT_Type->SetText(TypeText);
			}
		}

		if (TXT_Capacity)
		{
			FString CapacityStr = FString::Printf(TEXT("%.0f Capacity"), ItemInfo->ItemWeight);
			TXT_Capacity->SetText(FText::FromString(CapacityStr));
		}
	}
	else
	{
		// ID는 있는데 테이블에 정보가 없는 경우에도 안전하게 숨깁니다.
		SetVisibility(ESlateVisibility::Collapsed);
	}
}