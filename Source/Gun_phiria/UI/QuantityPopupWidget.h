#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InputCoreTypes.h"
#include "QuantityPopupWidget.generated.h"

// 전방 선언 (컴파일 속도 향상)
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
	// 블루프린트의 Event Construct와 동일한 역할을 하는 C++ 함수
	virtual void NativeConstruct() override;

	// ==========================================
	// [핵심] 블루프린트 UI와 연결될 변수들!
	// 반드시 블루프린트 디자이너에서 설정한 이름과 변수명이 완전히 똑같아야 합니다.
	// ==========================================
	UPROPERTY(meta = (BindWidget))
	USlider* Slider_Quantity;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Txt_CurrentAmount;

	UPROPERTY(meta = (BindWidget))
	UButton* Btn_Confirm;

	UPROPERTY(meta = (BindWidget))
	UButton* Btn_Cancel;

	// 실시간 총가격을 보여줄 텍스트 블록 바인딩
	UPROPERTY(meta = (BindWidget))
	UTextBlock* Txt_TotalPrice;

	// 골드 부족 시 띄워줄 경고 텍스트 바인딩
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* Txt_Warning;

	// 위젯에서 키보드가 눌렸을 때 발생하는 이벤트
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	// 내부 데이터를 저장할 변수들
	FName TargetItemID;
	int32 MaxQuantity;
	int32 SelectedQuantity;

	// 메인 위젯의 함수를 호출하기 위한 포인터
	UPROPERTY()
	UInventoryMainWidget* MainWidgetRef;

	// 컴포넌트 이벤트에 묶어줄(Bind) 콜백 함수들
	UFUNCTION()
	void OnSliderValueChanged(float Value);

	UFUNCTION()
	void OnConfirmClicked();

	UFUNCTION()
	void OnCancelClicked();

	bool bIsBuyingAction; // 현재 액션이 구매인지 판매인지 저장

	int32 UnitPrice; // 아이템 1개당 가격 저장용

	int32 AffordableQuantity; // 내 골드로 살 수 있는 최대 수량

	UFUNCTION()
	void OnSliderMouseCaptureEnd();
};