#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DropZoneWidget.h"
#include "InventoryMainWidget.generated.h"

class UScrollBox;
class APickupItemBase;
class UItemSlotWidget;
class UDragVisualWidget;
class UImage;
class UTextBlock;
class UPanelWidget;
class UItemTooltipWidget;
class UQuantityPopupWidget;
class AShopNPC;
class UItemDragOperation;

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
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ForceNearbyRefresh();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RefreshInventory();

	void ShowTooltip(FName ItemID, UDataTable* DataTable);
	void HideTooltip();

	UFUNCTION(BlueprintCallable, Category = "UI|Equipment")
	void UpdateEquipmentUI();

	void HandleItemDrop(UItemDragOperation* Operation, EDropZoneType TargetZone);

	UPROPERTY(BlueprintReadWrite, Category = "Inventory State")
	EInventoryMode CurrentMode = EInventoryMode::IM_Normal;

	void BuyItem(FName ItemID);
	void SellItem(FName ItemID);

	void StartNearbyTimer();
	void StopNearbyTimer();

	UPROPERTY(BlueprintReadWrite, Category = "Shop")
	TObjectPtr<AShopNPC> CurrentShopNPC;

	UFUNCTION(BlueprintCallable, Category = "Shop")
	void OpenShopMode(const TMap<FName, int32>& ShopItems);

	void UpdateShopUI(const TMap<FName, int32>& ShopItems);

	void PromptQuantitySelection(FName ItemID, int32 MaxAvailable, bool bIsBuying);

	UFUNCTION(BlueprintCallable, Category = "Shop")
	void ConfirmBuyItem(FName ItemID, int32 AmountToBuy);

	UFUNCTION(BlueprintCallable, Category = "Shop")
	void ConfirmSellItem(FName ItemID, int32 AmountToSell);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void UpdateCurrencyUI();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> VicinityScrollBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> InventoryScrollBox;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory | Settings")
	TSubclassOf<UUserWidget> ItemSlotWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory | Settings")
	TSubclassOf<UItemTooltipWidget> TooltipClass;

	UPROPERTY(EditDefaultsOnly, Category = "Drag And Drop")
	TSubclassOf<UDragVisualWidget> DragVisualClass;

	UPROPERTY(EditAnywhere, Category = "Shop UI")
	TSubclassOf<UQuantityPopupWidget> QuantityPopupClass;

	// === 위젯 바인딩 포인터 ===
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UItemSlotWidget> WBP_VestSlot;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UItemSlotWidget> WBP_HelmetSlot;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UItemSlotWidget> WBP_BackpackSlot;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UImage> IMG_CharacterPreview;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Txt_GoldAmount;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Txt_SapphireAmount;

	// === 무기 파츠 ===
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UItemSlotWidget> WBP_Weapon1Slot;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UItemSlotWidget> WBP_AttachmentSlot1_Scope;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UItemSlotWidget> WBP_AttachmentSlot1_Muzzle;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UItemSlotWidget> WBP_AttachmentSlot1_Magazine;
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadWrite, Category = "UI|Weapon") TObjectPtr<UPanelWidget> Container_Weapon1_Parts;

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UItemSlotWidget> WBP_Weapon2Slot;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UItemSlotWidget> WBP_AttachmentSlot2_Scope;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UItemSlotWidget> WBP_AttachmentSlot2_Muzzle;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UItemSlotWidget> WBP_AttachmentSlot2_Magazine;
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadWrite, Category = "UI|Weapon") TObjectPtr<UPanelWidget> Container_Weapon2_Parts;

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UItemSlotWidget> WBP_PistolSlot;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UItemSlotWidget> WBP_ThrowableSlot;

private:
	void CheckNearbyItems();
	void UpdateNearbyUI(const TArray<APickupItemBase*>& NearbyItems);

	FTimerHandle NearbyCheckTimer;
	int32 LastNearbyCount = -1;

	UPROPERTY()
	TObjectPtr<UItemTooltipWidget> CachedTooltip;

	FName DraggedEquipmentID;
};