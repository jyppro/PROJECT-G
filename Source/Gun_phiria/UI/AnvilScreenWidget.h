#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AnvilScreenWidget.generated.h"

struct FAnvilRewardData;

UCLASS()
class GUN_PHIRIA_API UAnvilScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 모루 액터가 호출할 초기화 함수
	void SetupAnvilUI(const TArray<FAnvilRewardData*>& Rewards);

	// UI 종료 및 입력 복구
	void CloseAnvilUI();

protected:
	// 슬롯들을 세로로 나열할 컨테이너
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UVerticalBox> ChoiceContainer;

	// 에디터에서 할당할 슬롯 블루프린트 클래스 (WBP_AnvilChoiceSlot)
	UPROPERTY(EditAnywhere, Category = "Anvil UI")
	TSubclassOf<class UAnvilChoiceSlotWidget> ChoiceSlotClass;
};