#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DropZoneWidget.generated.h"

// 3가지 구역 정의
UENUM(BlueprintType)
enum class EDropZoneType : uint8
{
	Nearby,      // 바닥
	Backpack,    // 가방
	Equipment    // 장착
};

UCLASS()
class GUN_PHIRIA_API UDropZoneWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 이 구역이 어디인지 언리얼 에디터에서 선택할 수 있게 노출
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DropZone")
	EDropZoneType ZoneType;

protected:
	// 드롭 이벤트 발동 시 실행
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
};