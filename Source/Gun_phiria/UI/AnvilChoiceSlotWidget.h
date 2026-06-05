#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../Reward/RewardData.h" // 경로 주의 (FAnvilRewardData가 있는 곳)
#include "AnvilChoiceSlotWidget.generated.h"

class UAnvilScreenWidget;

UCLASS()
class GUN_PHIRIA_API UAnvilChoiceSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 메인 스크린에서 데이터를 넘겨받아 UI를 초기화하는 함수
	void SetupSlot(const FAnvilRewardData& InRewardData, UAnvilScreenWidget* InParentScreen);

protected:
	virtual void NativeConstruct() override;

	// --- 연동할 블루프린트 위젯 ---
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UButton> ChoiceButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UImage> IconImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UTextBlock> NameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UTextBlock> DescriptionText;

private:
	UFUNCTION()
	void OnButtonClicked();

	FAnvilRewardData StoredRewardData;
	TWeakObjectPtr<UAnvilScreenWidget> ParentScreen;
};