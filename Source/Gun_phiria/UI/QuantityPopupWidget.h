#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InputCoreTypes.h"
#include "QuantityPopupWidget.generated.h"

class USlider;
class UTextBlock;
class UButton;
class UInventoryMainWidget;

UCLASS()
class GUN_PHIRIA_API UQuantityPopupWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetupPopup(FName InItemID, int32 InMaxQuantity, int32 InAffordableQuantity, int32 InUnitPrice, bool bIsBuying, UInventoryMainWidget* InMainWidget);

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> Slider_Quantity;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_CurrentAmount;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Confirm;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Cancel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_TotalPrice;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_Warning;

private:
	FName TargetItemID;
	int32 MaxQuantity;
	int32 SelectedQuantity;
	int32 UnitPrice;
	int32 AffordableQuantity;
	bool bIsBuyingAction;

	UPROPERTY()
	TObjectPtr<UInventoryMainWidget> MainWidgetRef;

	UFUNCTION()
	void OnSliderValueChanged(float Value);

	UFUNCTION()
	void OnConfirmClicked();

	UFUNCTION()
	void OnCancelClicked();

	UFUNCTION()
	void OnSliderMouseCaptureEnd();
};