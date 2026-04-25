#include "CastBarWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Materials/MaterialInstanceDynamic.h"

void UCastBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IMG_RadialBar)
	{
		RadialMaterial = IMG_RadialBar->GetDynamicMaterial();
	}
	bIsCasting = false;
}

void UCastBarWidget::StartCast(float Duration, UTexture2D* ItemIcon, bool bInUseWarningColor, float InWarningTimeThreshold)
{
	TotalDuration = Duration;
	TimeRemaining = Duration;
	bIsCasting = true;

	bUseWarningColor = bInUseWarningColor;
	WarningTimeThreshold = InWarningTimeThreshold;

	// 아이콘 텍스처 설정 및 가시성 처리 (삼항 연산자로 간결화)
	if (IMG_ItemIcon)
	{
		if (ItemIcon) IMG_ItemIcon->SetBrushFromTexture(ItemIcon);
		IMG_ItemIcon->SetVisibility(ItemIcon ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (RadialMaterial)
	{
		RadialMaterial->SetVectorParameterValue(FName("Color"), FLinearColor::White);
	}

	UpdateTimerUI(TimeRemaining, FLinearColor::White);
}

void UCastBarWidget::StopCast()
{
	bIsCasting = false;
	SetVisibility(ESlateVisibility::Collapsed);
}

void UCastBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 캐스팅 중이 아니거나 시간이 모두 소진되었으면 즉시 리턴
	if (!bIsCasting || TimeRemaining <= 0.0f) return;

	TimeRemaining = FMath::Max(0.0f, TimeRemaining - InDeltaTime);

	// 조건부 색상 로직 (삼항 연산자)
	FLinearColor TargetColor = (bUseWarningColor && TimeRemaining <= WarningTimeThreshold) ? FLinearColor::Red : FLinearColor::White;

	UpdateTimerUI(TimeRemaining, TargetColor);

	if (RadialMaterial && TotalDuration > 0.0f)
	{
		float Percent = 1.0f - (TimeRemaining / TotalDuration);
		RadialMaterial->SetScalarParameterValue(FName("Percent"), Percent);
		RadialMaterial->SetVectorParameterValue(FName("Color"), TargetColor);
	}

	if (TimeRemaining <= 0.0f)
	{
		StopCast(); // 중복 코드 제거, 기존 함수 재활용
	}
}

void UCastBarWidget::UpdateTimerUI(float CurrentTime, const FLinearColor& CurrentColor)
{
	if (TXT_Timer)
	{
		FNumberFormattingOptions Opts;
		Opts.MinimumFractionalDigits = 1;
		Opts.MaximumFractionalDigits = 1;

		TXT_Timer->SetText(FText::AsNumber(CurrentTime, &Opts));
		TXT_Timer->SetColorAndOpacity(FSlateColor(CurrentColor));
	}
}