#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "../Item/PickupItemBase.h"
#include "ItemDragOperation.generated.h"

// [수정] Blueprintable을 추가해야 블루프린트에서 노드로 생성할 수 있습니다!
UCLASS(Blueprintable)
class GUN_PHIRIA_API UItemDragOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	// [수정] ExposeOnSpawn을 추가해야 생성 노드에 핀이 노출됩니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DragDrop", meta = (ExposeOnSpawn = "true"))
	FName DraggedItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DragDrop", meta = (ExposeOnSpawn = "true"))
	bool bIsFromGround = false;

	// [추가] 장착 슬롯에서 끌어온 것인지 판별
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DragDrop", meta = (ExposeOnSpawn = "true"))
	bool bIsFromEquip = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DragDrop", meta = (ExposeOnSpawn = "true"))
	APickupItemBase* DraggedActor = nullptr;
};