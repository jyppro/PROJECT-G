#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryMainWidget.generated.h"

// 전방 선언을 통해 헤더 포함을 최소화합니다.
class UScrollBox;
class APickupItemBase;
class UItemSlotWidget; // 이전에 만든 슬롯 위젯 클래스

UCLASS()
class GUN_PHIRIA_API UInventoryMainWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 인벤토리를 열 때 호출하여 즉시 새로고침하는 함수
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ForceNearbyRefresh();

	// 가방(Backpack) 아이템 리스트를 새로고침하는 함수 (캐릭터에서 호출 가능)
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RefreshInventory();

	// 슬롯에서 호출할 함수
	void ShowTooltip(FName ItemID, UDataTable* DataTable);
	void HideTooltip();

protected:
	// 초기화 및 타이머 설정
	virtual void NativeConstruct() override;

	// 타이머 해제 및 정리
	virtual void NativeDestruct() override;

	// ==========================================
	// UI 컴포넌트 바인딩 (이름이 WBP_InventoryMain의 위젯 이름과 일치해야 함)
	// ==========================================

	// 주변 아이템 스크롤 박스
	UPROPERTY(meta = (BindWidget))
	UScrollBox* VicinityScrollBox;

	// 가방 아이템 스크롤 박스
	UPROPERTY(meta = (BindWidget))
	UScrollBox* InventoryScrollBox;

	// ==========================================
	// 위젯 설정
	// ==========================================

	// 슬롯으로 생성할 위젯 클래스 (Class Defaults에서 WBP_ItemSlot 선택)
	UPROPERTY(EditDefaultsOnly, Category = "Inventory | Settings")
	TSubclassOf<UUserWidget> ItemSlotWidgetClass;

	// 에디터에서 설정할 툴팁 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Inventory | Settings")
	TSubclassOf<class UItemTooltipWidget> TooltipClass;

private:
	// 주기적으로 주변 아이템을 검사하는 함수
	void CheckNearbyItems();

	// 실제 UI 리스트를 다시 그리는 내부 로직 (블루프린트 이벤트 대신 C++ 함수 사용)
	void UpdateNearbyUI(const TArray<APickupItemBase*>& NearbyItems);

	// 타이머 및 상태 관리 변수
	FTimerHandle NearbyCheckTimer;
	int32 LastNearbyCount = -1;

	UPROPERTY()
	UItemTooltipWidget* CachedTooltip;
};