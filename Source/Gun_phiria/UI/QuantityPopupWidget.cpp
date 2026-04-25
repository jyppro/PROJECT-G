#include "QuantityPopupWidget.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "InventoryMainWidget.h" 

void UQuantityPopupWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 본체인 UUserWidget은 여전히 이 함수를 쓸 수 있으므로 남겨둡니다.
	SetIsFocusable(true);

	if (Slider_Quantity)
	{
		Slider_Quantity->OnValueChanged.AddDynamic(this, &UQuantityPopupWidget::OnSliderValueChanged);
		Slider_Quantity->OnMouseCaptureEnd.AddDynamic(this, &UQuantityPopupWidget::OnSliderMouseCaptureEnd);
	}

	if (Btn_Confirm)
	{
		Btn_Confirm->OnClicked.AddDynamic(this, &UQuantityPopupWidget::OnConfirmClicked);
	}

	if (Btn_Cancel)
	{
		Btn_Cancel->OnClicked.AddDynamic(this, &UQuantityPopupWidget::OnCancelClicked);
	}
}

void UQuantityPopupWidget::SetupPopup(FName InItemID, int32 InMaxQuantity, int32 InAffordableQuantity, int32 InUnitPrice, bool bIsBuying, UInventoryMainWidget* InMainWidget)
{
	TargetItemID = InItemID;
	MaxQuantity = InMaxQuantity;
	AffordableQuantity = InAffordableQuantity;
	UnitPrice = InUnitPrice;
	bIsBuyingAction = bIsBuying;
	MainWidgetRef = InMainWidget;

	if (Slider_Quantity)
	{
		Slider_Quantity->SetMinValue(1.0f);
		Slider_Quantity->SetMaxValue(FMath::Max(1.0f, static_cast<float>(MaxQuantity)));
		Slider_Quantity->SetStepSize(1.0f);
		Slider_Quantity->MouseUsesStep = true;
		Slider_Quantity->SetValue(1.0f);
	}

	OnSliderValueChanged(1.0f);
}

void UQuantityPopupWidget::OnSliderValueChanged(float Value)
{
	SelectedQuantity = FMath::RoundToInt(Value);

	bool bIsExceeding = (bIsBuyingAction && SelectedQuantity > AffordableQuantity);

	if (Txt_Warning) Txt_Warning->SetVisibility(bIsExceeding ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (Btn_Confirm) Btn_Confirm->SetIsEnabled(!bIsExceeding);

	if (Txt_CurrentAmount)
	{
		Txt_CurrentAmount->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), SelectedQuantity, MaxQuantity)));
	}

	if (Txt_TotalPrice)
	{
		Txt_TotalPrice->SetText(FText::FromString(FString::Printf(TEXT("Total Price : %d G"), UnitPrice * SelectedQuantity)));
	}
}

void UQuantityPopupWidget::OnConfirmClicked()
{
	if (MainWidgetRef)
	{
		if (bIsBuyingAction) MainWidgetRef->ConfirmBuyItem(TargetItemID, SelectedQuantity);
		else MainWidgetRef->ConfirmSellItem(TargetItemID, SelectedQuantity);
	}
	RemoveFromParent();
}

void UQuantityPopupWidget::OnCancelClicked()
{
	RemoveFromParent();
}

FReply UQuantityPopupWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Enter)
	{
		if (Btn_Confirm && Btn_Confirm->GetIsEnabled())
		{
			OnConfirmClicked();
			return FReply::Handled();
		}
	}
	else if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		OnCancelClicked();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UQuantityPopupWidget::OnSliderMouseCaptureEnd()
{
	SetKeyboardFocus();
}