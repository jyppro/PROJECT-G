#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryMainWidget.generated.h"

struct FUpdateSlotParams
{
	FName ItemID;
	int32 Quantity;
};

UCLASS()
class GUN_PHIRIA_API UInventoryMainWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 인벤토리를 열 때 호출할 강제 새로고침 함수
	void ForceNearbyRefresh();

protected:
	// UI가 처음 화면에 생성될 때 실행되는 함수 (블루프린트의 Event Construct와 동일)
	virtual void NativeConstruct() override;

	// UI가 화면에서 사라질 때 실행 (타이머 정리용)
	virtual void NativeDestruct() override;

	// 마법의 매크로! 블루프린트에 있는 똑같은 이름의 스크롤 박스를 자동으로 연결해줌
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	class UScrollBox* VicinityScrollBox;

	// 가방 스크롤 박스도 미리 연결해두면 좋아!
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	class UScrollBox* InventoryScrollBox;

	// 슬롯으로 생성할 위젯 클래스 (에디터에서 WBP_ItemSlot을 넣어줄 칸)
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UUserWidget> ItemSlotWidgetClass;

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void UpdateNearbyUI(const TArray<class APickupItemBase*>& NearbyItems);

private:
	// 타이머 핸들과 이전 아이템 개수 기억용 변수
	FTimerHandle NearbyCheckTimer;
	int32 LastNearbyCount = -1;

	// 타이머가 주기적으로 실행할 함수
	void CheckNearbyItems();

	// 실제로 UI를 지우고 새로 그리는 함수
	// void RefreshNearbyUI(const TArray<class APickupItemBase*>& NearbyItems);
};