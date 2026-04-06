#include "QuantityPopupWidget.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "InventoryMainWidget.h" // 네 프로젝트 경로에 맞게 인클루드 해주세요!

void UQuantityPopupWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// [추가] 이 위젯이 키보드 포커스를 받을 수 있도록 허용합니다.
	SetIsFocusable(true);

	if (Slider_Quantity)
	{
		// ==========================================
		// [수정된 부분] 함수 대신 변수에 직접 false를 대입합니다!
		// ==========================================
		Slider_Quantity->IsFocusable = false;

		Slider_Quantity->OnValueChanged.AddDynamic(this, &UQuantityPopupWidget::OnSliderValueChanged);
		Slider_Quantity->OnMouseCaptureEnd.AddDynamic(this, &UQuantityPopupWidget::OnSliderMouseCaptureEnd);
	}

	if (Btn_Confirm)
	{
		// 버튼들도 마찬가지로 변수에 직접 대입합니다.
		Btn_Confirm->IsFocusable = false;
		Btn_Confirm->OnClicked.AddDynamic(this, &UQuantityPopupWidget::OnConfirmClicked);
	}

	if (Btn_Cancel)
	{
		Btn_Cancel->IsFocusable = false;
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
		Slider_Quantity->SetMaxValue(FMath::Max(1.0f, (float)MaxQuantity)); // 상점 재고 끝까지 당길 수 있게 유지!
		Slider_Quantity->SetStepSize(1.0f);
		Slider_Quantity->MouseUsesStep = true;
		Slider_Quantity->SetValue(1.0f);
	}

	// [핵심] 초기 텍스트 및 버튼 상태 세팅을 위해 1개일 때의 이벤트를 강제로 한 번 호출합니다.
	OnSliderValueChanged(1.0f);
}

void UQuantityPopupWidget::OnSliderValueChanged(float Value)
{
	SelectedQuantity = FMath::RoundToInt(Value);

	// ==========================================================
	// [수정된 부분] 슬라이더는 자유롭게 두고, 경고문구와 '확인 버튼'만 제어합니다!
	// ==========================================================
	if (bIsBuyingAction && SelectedQuantity > AffordableQuantity)
	{
		// 돈이 부족한 구간(예: 3개부터)에 도달하면 경고문구 표시 & 확인 버튼 클릭 금지!
		if (Txt_Warning) Txt_Warning->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (Btn_Confirm) Btn_Confirm->SetIsEnabled(false);
	}
	else
	{
		// 살 수 있는 구간(예: 1~2개)에서는 경고문구 숨김 & 확인 버튼 활성화!
		if (Txt_Warning) Txt_Warning->SetVisibility(ESlateVisibility::Collapsed);
		if (Btn_Confirm) Btn_Confirm->SetIsEnabled(true);
	}

	// 텍스트 업데이트 (현재수량 / 최대수량)
	if (Txt_CurrentAmount)
	{
		FString AmountStr = FString::Printf(TEXT("%d / %d"), SelectedQuantity, MaxQuantity);
		Txt_CurrentAmount->SetText(FText::FromString(AmountStr));
	}

	// 총 가격 업데이트
	if (Txt_TotalPrice)
	{
		int32 TotalCost = UnitPrice * SelectedQuantity;
		FString PriceStr = FString::Printf(TEXT("Total Price : %d G"), TotalCost);
		Txt_TotalPrice->SetText(FText::FromString(PriceStr));
	}
}

void UQuantityPopupWidget::OnConfirmClicked()
{
	if (MainWidgetRef)
	{
		if (bIsBuyingAction)
		{
			// 구매일 경우
			MainWidgetRef->ConfirmBuyItem(TargetItemID, SelectedQuantity);
		}
		else
		{
			// 판매일 경우 (아래에서 만들 함수)
			MainWidgetRef->ConfirmSellItem(TargetItemID, SelectedQuantity);
		}
	}

	RemoveFromParent();
}

void UQuantityPopupWidget::OnCancelClicked()
{
	RemoveFromParent();
}

FReply UQuantityPopupWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// 눌린 키가 Enter일 때
	if (InKeyEvent.GetKey() == EKeys::Enter)
	{
		// [핵심] 버튼이 활성화(클릭 가능) 상태일 때만 구매를 진행합니다! (돈이 부족하면 안 눌리도록)
		if (Btn_Confirm && Btn_Confirm->GetIsEnabled())
		{
			OnConfirmClicked();
			return FReply::Handled(); // 입력을 처리했음을 엔진에 알림
		}
	}
	// 눌린 키가 ESC일 때
	else if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		OnCancelClicked();
		return FReply::Handled();
	}

	// 그 외의 키 입력은 부모 클래스로 넘김
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UQuantityPopupWidget::OnSliderMouseCaptureEnd()
{
	// 슬라이더 조작(드래그)이 끝나는 즉시, 
	// 팝업창 본체(this)로 키보드 포커스를 강제로 다시 가져옵니다!
	SetKeyboardFocus();
}