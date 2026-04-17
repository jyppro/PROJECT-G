#include "PlayerStatusWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UPlayerStatusWidget::UpdateBackpackCapacity(float Percent)
{
	if (PB_Backpack)
	{
		PB_Backpack->SetPercent(FMath::Clamp(Percent, 0.0f, 1.0f));
	}
}

void UPlayerStatusWidget::UpdateHelmetDurability(float Percent)
{
	if (PB_Helmet)
	{
		PB_Helmet->SetPercent(FMath::Clamp(Percent, 0.0f, 1.0f));
		// 내구도가 0이면 아예 숨기고 싶을 경우 아래 주석 해제
		// PB_Helmet->SetVisibility(Percent > 0.0f ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}

void UPlayerStatusWidget::UpdateVestDurability(float Percent)
{
	if (PB_Vest)
	{
		PB_Vest->SetPercent(FMath::Clamp(Percent, 0.0f, 1.0f));
	}
}

void UPlayerStatusWidget::UpdateAmmo(int32 CurrentAmmo, int32 ReserveAmmo)
{
	// 1. 현재 탄창에 있는 탄약 수 업데이트 (예: "34")
	if (TXT_CurrentAmmo)
	{
		TXT_CurrentAmmo->SetText(FText::AsNumber(CurrentAmmo));
	}

	// 2. 가방에 있는 전체 탄약 수 업데이트 (예: "120")
	if (TXT_BackpackAmmo)
	{
		TXT_BackpackAmmo->SetText(FText::AsNumber(ReserveAmmo));
	}
}

void UPlayerStatusWidget::UpdateSpeedBoost(float Percent)
{
	if (PB_SpeedBoost)
	{
		PB_SpeedBoost->SetPercent(FMath::Clamp(Percent, 0.0f, 1.0f));
		// 부스트가 없을 때는 아이콘 자체를 숨김 처리 (배틀그라운드 방식)
		PB_SpeedBoost->SetVisibility(Percent > 0.0f ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UPlayerStatusWidget::UpdateHealBoost(float Percent)
{
	if (PB_HealBoost)
	{
		PB_HealBoost->SetPercent(FMath::Clamp(Percent, 0.0f, 1.0f));
		PB_HealBoost->SetVisibility(Percent > 0.0f ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UPlayerStatusWidget::UpdateHealth(float Percent)
{
	if (PB_Health)
	{
		PB_Health->SetPercent(FMath::Clamp(Percent, 0.0f, 1.0f));
	}
}