#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../Reward/RewardData.h"
#include "AnvilScreenWidget.generated.h"

class UVerticalBox;
class UButton; // [추가]

UCLASS()
class GUN_PHIRIA_API UAnvilScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// [수정] 무기 배열과 업그레이드 배열을 동시에 받고, 기본으로 띄울 모드를 결정합니다.
	void SetupAnvilUI(const TArray<FAnvilRewardData*>& InWeaponRewards, const TArray<FAnvilRewardData*>& InUpgradeRewards, EAnvilRewardType DefaultMode);

	void CloseAnvilUI();

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> ChoiceContainer;

	UPROPERTY(EditAnywhere, Category = "Anvil UI")
	TSubclassOf<class UAnvilChoiceSlotWidget> ChoiceSlotClass;

	// [추가] 탭 버튼 2개
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> WeaponModeBtn;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> UpgradeModeBtn;

private:
	// [추가] 버튼 클릭 이벤트
	UFUNCTION()
	void OnWeaponModeClicked();

	UFUNCTION()
	void OnUpgradeModeClicked();

	// [추가] 컨테이너를 비우고 특정 배열의 데이터를 화면에 그려주는 함수
	void DisplayRewards(const TArray<FAnvilRewardData*>& RewardsToDisplay);

	// [추가] 상호작용 시 전달받은 보상들을 캐싱(기억)해둘 배열
	TArray<FAnvilRewardData*> CachedWeaponRewards;
	TArray<FAnvilRewardData*> CachedUpgradeRewards;
};