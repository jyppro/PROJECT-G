#include "DropZoneWidget.h"
#include "../UI/ItemDragOperation.h" // 경로 맞춰주기
#include "InventoryMainWidget.h"

bool UDropZoneWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);

	UItemDragOperation* ItemDrag = Cast<UItemDragOperation>(InOperation);

	// 이 구역을 소유하고 있는 메인 인벤토리 위젯을 찾음
	UInventoryMainWidget* MainUI = GetTypedOuter<UInventoryMainWidget>();

	if (ItemDrag && MainUI)
	{
		// 메인 UI의 뇌(HandleItemDrop)로 데이터를 넘겨버림!
		MainUI->HandleItemDrop(ItemDrag, ZoneType);
		return true; // 드롭 성공!
	}

	return false;
}