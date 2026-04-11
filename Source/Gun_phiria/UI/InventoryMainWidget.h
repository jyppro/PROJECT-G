#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "DropZoneWidget.h"
#include "InventoryMainWidget.generated.h"

// 전방 선언을 통해 헤더 포함을 최소화합니다.
class UScrollBox;
class APickupItemBase;
class UItemSlotWidget; // 이전에 만든 슬롯 위젯 클래스
class UDragVisualWidget;

UENUM(BlueprintType)
enum class EInventoryMode : uint8
{
	IM_Normal UMETA(DisplayName = "Normal Looting"),
	IM_Shop   UMETA(DisplayName = "Shop Mode")
};

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

	UFUNCTION(BlueprintCallable, Category = "UI|Equipment")
	void UpdateEquipmentUI();

	// 드롭존이 호출할 마스터 처리 함수
	void HandleItemDrop(class UItemDragOperation* Operation, EDropZoneType TargetZone);

	// 현재 인벤토리 상태
	UPROPERTY(BlueprintReadWrite, Category = "Inventory State")
	EInventoryMode CurrentMode = EInventoryMode::IM_Normal;

	// 구매/판매 처리 함수
	void BuyItem(FName ItemID);
	void SellItem(FName ItemID);

	void StartNearbyTimer();
	void StopNearbyTimer();

	UPROPERTY(BlueprintReadWrite, Category = "Shop")
	class AShopNPC* CurrentShopNPC;

	// TArray에서 TMap으로 변경
	UFUNCTION(BlueprintCallable, Category = "Shop")
	void OpenShopMode(const TMap<FName, int32>& ShopItems);

	void UpdateShopUI(const TMap<FName, int32>& ShopItems);

	void PromptQuantitySelection(FName ItemID, int32 MaxAvailable, bool bIsBuying);

	// 팝업창에서 수량을 정하고 '확인'을 눌렀을 때 C++로 호출될 함수
	UFUNCTION(BlueprintCallable, Category = "Shop")
	void ConfirmBuyItem(FName ItemID, int32 AmountToBuy);

	UFUNCTION(BlueprintCallable, Category = "Shop")
	void ConfirmSellItem(FName ItemID, int32 AmountToSell);

	// 재화 UI를 업데이트하는 함수
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void UpdateCurrencyUI();

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

	UPROPERTY(meta = (BindWidget))
	UItemSlotWidget* WBP_VestSlot;

	UPROPERTY(meta = (BindWidget))
	UItemSlotWidget* WBP_HelmetSlot;

	UPROPERTY(meta = (BindWidget))
	UItemSlotWidget* WBP_BackpackSlot;

	UPROPERTY(meta = (BindWidget))
	UImage* IMG_CharacterPreview;

	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	UPROPERTY(EditDefaultsOnly, Category = "Drag And Drop")
	TSubclassOf<UDragVisualWidget> DragVisualClass;

	// [추가] 드래그가 어떤 드롭존에도 들어가지 못하고 취소되었을 때 호출되는 함수
	virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	UPROPERTY(EditAnywhere, Category = "Shop UI")
	TSubclassOf<class UQuantityPopupWidget> QuantityPopupClass;

	// 재화 표시용 텍스트 바인딩
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* Txt_GoldAmount;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* Txt_SapphireAmount;

	// ==========================================
	// --- Combat (전투) 슬롯 바인딩 ---
	// ==========================================

	// === 1번 무기 (Primary Weapon) ===
	UPROPERTY(meta = (BindWidget))
	class UItemSlotWidget* WBP_Weapon1Slot;

	UPROPERTY(meta = (BindWidgetOptional))
	class UItemSlotWidget* WBP_AttachmentSlot1_Scope;

	UPROPERTY(meta = (BindWidgetOptional))
	class UItemSlotWidget* WBP_AttachmentSlot1_Muzzle;

	UPROPERTY(meta = (BindWidgetOptional))
	class UItemSlotWidget* WBP_AttachmentSlot1_Magazine;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadWrite, Category = "UI|Weapon")
	class UPanelWidget* Container_Weapon1_Parts;

	// === 2번 무기 (Secondary Weapon) ===
	UPROPERTY(meta = (BindWidget))
	class UItemSlotWidget* WBP_Weapon2Slot;

	UPROPERTY(meta = (BindWidgetOptional))
	class UItemSlotWidget* WBP_AttachmentSlot2_Scope;

	UPROPERTY(meta = (BindWidgetOptional))
	class UItemSlotWidget* WBP_AttachmentSlot2_Muzzle;

	UPROPERTY(meta = (BindWidgetOptional))
	class UItemSlotWidget* WBP_AttachmentSlot2_Magazine;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadWrite, Category = "UI|Weapon")
	class UPanelWidget* Container_Weapon2_Parts;

	// === 권총 (Pistol) ===
	UPROPERTY(meta = (BindWidget))
	class UItemSlotWidget* WBP_PistolSlot;

	// === 투척 무기 (Throwable) ===
	UPROPERTY(meta = (BindWidget))
	class UItemSlotWidget* WBP_ThrowableSlot;

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

	// 장착 칸에서 드래그를 시작할 때 어떤 장비를 잡았는지 기억하는 변수
	FName DraggedEquipmentID;
};