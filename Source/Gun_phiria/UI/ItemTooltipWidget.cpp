#include "ItemTooltipWidget.h"
#include "Components/TextBlock.h"
#include "Engine/DataTable.h"
#include "../component/InventoryComponent.h" // FItemData 구조체 위치
#include "Components/Image.h"

void UItemTooltipWidget::UpdateTooltip(FName ItemID, UDataTable* ItemDataTable)
{
	if (!ItemDataTable) return;

	static const FString ContextString(TEXT("Tooltip Lookup"));
	FItemData* ItemInfo = ItemDataTable->FindRow<FItemData>(ItemID, ContextString);

	if (ItemInfo)
	{
		if (TXT_Name) TXT_Name->SetText(ItemInfo->ItemName);
		if (TXT_Description) TXT_Description->SetText(ItemInfo->ItemDescription);

		// 1. 아이콘 이미지 설정
		if (IMG_ItemIcon && ItemInfo->ItemIcon)
		{
			IMG_ItemIcon->SetBrushFromTexture(ItemInfo->ItemIcon);
		}

		// 2. 타입 텍스트 변환 및 설정 (Consumable, Weapon 등)
		if (TXT_Type)
		{
			// 주의: EItemType 자리에 본인이 헤더 파일에 정의한 실제 Enum 이름을 적어주세요. 
			// (예: EItemType, E_ItemType 등)
			const UEnum* EnumPtr = StaticEnum<EItemType>();

			if (EnumPtr)
			{
				// Enum 값을 에디터에 설정한 Display Name(표시 이름)으로 변환합니다.
				FText TypeText = EnumPtr->GetDisplayNameTextByValue(static_cast<int64>(ItemInfo->ItemType));
				TXT_Type->SetText(TypeText);
			}
		}

		// 3. 용량/무게 텍스트
		if (TXT_Capacity)
		{
			FString CapacityStr = FString::Printf(TEXT("%.0f Capacity"), ItemInfo->ItemWeight);
			TXT_Capacity->SetText(FText::FromString(CapacityStr));
		}
	}
}