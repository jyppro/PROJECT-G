#include "AnvilScreenWidget.h"
#include "AnvilChoiceSlotWidget.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"

void UAnvilScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 버튼 이벤트 바인딩
	if (WeaponModeBtn) WeaponModeBtn->OnClicked.AddDynamic(this, &UAnvilScreenWidget::OnWeaponModeClicked);
	if (UpgradeModeBtn) UpgradeModeBtn->OnClicked.AddDynamic(this, &UAnvilScreenWidget::OnUpgradeModeClicked);
}

void UAnvilScreenWidget::SetupAnvilUI(const TArray<FAnvilRewardData*>& InWeaponRewards, const TArray<FAnvilRewardData*>& InUpgradeRewards, EAnvilRewardType DefaultMode)
{
	// 1. 전달받은 데이터를 UI 변수에 저장(캐싱)하여 유지시킵니다.
	CachedWeaponRewards = InWeaponRewards;
	CachedUpgradeRewards = InUpgradeRewards;

	// 2. 무기가 2개 꽉 차서 WeaponMode가 불가하면 버튼 비활성화
	if (CachedWeaponRewards.Num() == 0 && WeaponModeBtn)
	{
		WeaponModeBtn->SetIsEnabled(false);
	}

	// 3. 무기가 아예 없어서 UpgradeMode가 불가하면 버튼 비활성화
	if (CachedUpgradeRewards.Num() == 0 && UpgradeModeBtn)
	{
		UpgradeModeBtn->SetIsEnabled(false);
	}

	// 4. 최초에 화면에 띄울 모드 설정
	if (DefaultMode == EAnvilRewardType::WeaponUpgrade && CachedUpgradeRewards.Num() > 0)
	{
		DisplayRewards(CachedUpgradeRewards);
	}
	else
	{
		DisplayRewards(CachedWeaponRewards);
	}
}

void UAnvilScreenWidget::OnWeaponModeClicked()
{
	DisplayRewards(CachedWeaponRewards);
}

void UAnvilScreenWidget::OnUpgradeModeClicked()
{
	DisplayRewards(CachedUpgradeRewards);
}

void UAnvilScreenWidget::DisplayRewards(const TArray<FAnvilRewardData*>& RewardsToDisplay)
{
	if (!ChoiceContainer || !ChoiceSlotClass) return;

	// 기존 화면의 선택지들을 모두 지웁니다.
	ChoiceContainer->ClearChildren();

	for (FAnvilRewardData* Reward : RewardsToDisplay)
	{
		if (Reward)
		{
			UAnvilChoiceSlotWidget* NewSlot = CreateWidget<UAnvilChoiceSlotWidget>(this, ChoiceSlotClass);
			if (NewSlot)
			{
				NewSlot->SetupSlot(*Reward, this);
				UPanelSlot* PanelSlot = ChoiceContainer->AddChild(NewSlot);

				if (UVerticalBoxSlot* VertSlot = Cast<UVerticalBoxSlot>(PanelSlot))
				{
					VertSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 30.f));
				}
			}
		}
	}
}

void UAnvilScreenWidget::CloseAnvilUI()
{
	// 기존 Close 로직 동일 유지
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		UGameplayStatics::SetGamePaused(GetWorld(), false);
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
		PC->SetShowMouseCursor(false);
	}
	RemoveFromParent();
}