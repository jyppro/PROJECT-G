#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Engine/DataTable.h" // 데이터 테이블을 사용하기 위해 추가
#include "ItemSlotWidget.generated.h"

// 전방 선언 (헤더 포함을 줄여 컴파일 속도 향상)
class UImage;
class UTextBlock;
class APickupItemBase;

UCLASS()
class GUN_PHIRIA_API UItemSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 인벤토리에서 아이템 정보를 세팅할 때 호출할 함수
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetItemInfo(FName InItemID, int32 InQuantity);

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TSubclassOf<UUserWidget> TooltipWidgetClass;

	// 이 슬롯이 바닥에 있는 아이템인지 체크하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ItemSlot")
	bool bIsVicinitySlot = false;

	// [추가된 부분] 바닥에 있는 실제 아이템 액터를 가리키는 포인터
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ItemSlot")
	APickupItemBase* TargetItemActor = nullptr;

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// 마우스가 들어올 때
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// 마우스가 나갈 때 (이 줄이 빠져있었습니다!)
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ItemSlot")
	FName CurrentItemID;

	// ==========================================
	// C++ UI 바인딩 (이름이 블루프린트 UI 이름과 완전히 똑같아야 합니다!)
	// ==========================================

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UImage* IMG_ItemIcon;

	UPROPERTY(meta = (BindWidget))
	UImage* IMG_Background;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TXT_ItemName;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TXT_ItemQuantity;

	// ==========================================
	// 데이터 테이블 설정
	// ==========================================

	// 블루프린트 디테일 패널에서 사용할 데이터 테이블을 할당할 수 있도록 만듭니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory Data")
	UDataTable* ItemDataTable;
};