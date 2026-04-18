#include "PlayerStatusWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UPlayerStatusWidget::UpdateBackpackCapacity(float Percent)
{
	if (PB_Backpack)
	{
		PB_Backpack->SetPercent(FMath::Clamp(Percent, 0.0f, 1.0f));

		// [추가] 용량이 100% (1.0) 이상이면 빨간색, 아니면 기본 하얀색으로 변경
		FLinearColor FillColor = (Percent >= 1.0f) ? FLinearColor::Red : FLinearColor::White;
		PB_Backpack->SetFillColorAndOpacity(FillColor);
	}
}

void UPlayerStatusWidget::UpdateHelmetDurability(float Percent)
{
	if (PB_Helmet)
	{
		PB_Helmet->SetPercent(FMath::Clamp(Percent, 0.0f, 1.0f));

		// [추가] 내구도가 30% (0.3) 이하로 떨어지면 빨간색, 아니면 기본 하얀색으로 변경
		// (장착 해제 상태여서 0%일 때는 어차피 Fill 이미지가 보이지 않으므로 괜찮습니다.)
		FLinearColor FillColor = (Percent <= 0.3f) ? FLinearColor::Red : FLinearColor::White;
		PB_Helmet->SetFillColorAndOpacity(FillColor);
	}
}

void UPlayerStatusWidget::UpdateVestDurability(float Percent)
{
	if (PB_Vest)
	{
		PB_Vest->SetPercent(FMath::Clamp(Percent, 0.0f, 1.0f));

		// [추가] 조끼 내구도 30% 이하일 때 빨간색 경고
		FLinearColor FillColor = (Percent <= 0.3f) ? FLinearColor::Red : FLinearColor::White;
		PB_Vest->SetFillColorAndOpacity(FillColor);
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
		float ClampedPercent = FMath::Clamp(Percent, 0.0f, 1.0f);
		PB_Health->SetPercent(ClampedPercent);

		// [수정 포인트 1] 색상을 미리 변수로 정의하여 50% 구간의 색상을 완벽하게 일치시킵니다.
		FLinearColor CustomWhite = FLinearColor::White;
		FLinearColor CustomYellow = FLinearColor(1.0f, 0.95f, 0.3f, 1.0f);
		FLinearColor CustomRed = FLinearColor(0.55f, 0.0f, 0.0f, 1.0f);

		FLinearColor FillColor;

		// 1. 체력이 50% ~ 100% 사이일 때: 노란색 -> 하얀색으로 부드럽게 전환
		if (ClampedPercent >= 0.5f)
		{
			float Alpha = (ClampedPercent - 0.5f) * 2.0f;
			FillColor = FMath::Lerp(CustomYellow, CustomWhite, Alpha);
		}
		// 2. 체력이 20% ~ 50% 미만 사이일 때: 빨간색 -> 노란색으로 부드럽게 전환
		else if (ClampedPercent > 0.2f)
		{
			float Alpha = (ClampedPercent - 0.2f) * (1.0f / 0.3f);
			FillColor = FMath::Lerp(CustomRed, CustomYellow, Alpha);
		}
		// 3. [수정 포인트 2] 체력이 20% 이하일 때: 마이너스 알파값이 나오지 않게 무조건 빨간색 고정!
		else
		{
			FillColor = CustomRed;
		}

		PB_Health->SetFillColorAndOpacity(FillColor);
	}
}