#include "AnvilScreenWidget.h"
#include "AnvilChoiceSlotWidget.h"
#include "Components/VerticalBox.h"
#include "Kismet/GameplayStatics.h"
#include "../Reward/RewardData.h"
#include "Components/VerticalBoxSlot.h"

void UAnvilScreenWidget::SetupAnvilUI(const TArray<FAnvilRewardData*>& Rewards)
{
	if (!ChoiceContainer || !ChoiceSlotClass) return;

	ChoiceContainer->ClearChildren();

	for (FAnvilRewardData* Reward : Rewards)
	{
		if (Reward)
		{
			UAnvilChoiceSlotWidget* NewSlot = CreateWidget<UAnvilChoiceSlotWidget>(this, ChoiceSlotClass);
			if (NewSlot)
			{
				NewSlot->SetupSlot(*Reward, this);

				// 1. 위젯을 컨테이너에 넣고, 그 슬롯(패널) 정보를 받아옵니다.
				UPanelSlot* PanelSlot = ChoiceContainer->AddChild(NewSlot);

				// 2. 이 슬롯이 Vertical Box 슬롯이 맞다면 캐스팅합니다.
				if (UVerticalBoxSlot* VertSlot = Cast<UVerticalBoxSlot>(PanelSlot))
				{
					// 3. 패딩 설정: FMargin(Left, Top, Right, Bottom) 순서입니다.
					// 아래쪽(Bottom)에만 30의 여백을 줍니다.
					VertSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 30.f));
				}
			}
		}
	}
}

void UAnvilScreenWidget::CloseAnvilUI()
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		// UI 닫을 때 게임 일시정지도 함께 해제
		UGameplayStatics::SetGamePaused(GetWorld(), false);

		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
		PC->SetShowMouseCursor(false);
	}

	RemoveFromParent();
}