#include "CastBarWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Materials/MaterialInstanceDynamic.h"

void UCastBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 게임이 시작될 때 미리 다이내믹 머티리얼을 가져와 캐싱해 둡니다.
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

	// [중요] 시전을 시작할 때는 무조건 기본색(White)으로 초기화!
	if (TXT_Timer) TXT_Timer->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	if (RadialMaterial) RadialMaterial->SetVectorParameterValue(FName("Color"), FLinearColor::White);

	// 1. 아이템 아이콘 텍스처 설정
	if (IMG_ItemIcon)
	{
		if (ItemIcon)
		{
			IMG_ItemIcon->SetBrushFromTexture(ItemIcon);
			IMG_ItemIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible); // 아이콘 보이게
		}
		else
		{
			// [수정] ItemIcon이 nullptr(없음)이면 이미지를 완전히 숨깁니다!
			IMG_ItemIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// 2. 초기 텍스트 설정 (예: 6.0)
	if (TXT_Timer)
	{
		FNumberFormattingOptions Opts;
		Opts.MinimumFractionalDigits = 1;
		Opts.MaximumFractionalDigits = 1;
		TXT_Timer->SetText(FText::AsNumber(TimeRemaining, &Opts));
	}
}

void UCastBarWidget::StopCast()
{
	// 캐스팅 상태를 끄고, 위젯을 화면에서 숨깁니다.
	bIsCasting = false;
	SetVisibility(ESlateVisibility::Collapsed);
}

void UCastBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bIsCasting && TimeRemaining > 0.0f)
	{
		TimeRemaining -= InDeltaTime;
		if (TimeRemaining < 0.0f) TimeRemaining = 0.0f;

		// --- [수정] 조건부 색상 로직 ---
		FLinearColor TargetColor = FLinearColor::White;
		if (bUseWarningColor && TimeRemaining <= WarningTimeThreshold)
		{
			TargetColor = FLinearColor::Red;
		}

		// 1. 텍스트 업데이트 및 색상 설정
		if (TXT_Timer)
		{
			FNumberFormattingOptions Opts;
			Opts.MinimumFractionalDigits = 1;
			Opts.MaximumFractionalDigits = 1;
			TXT_Timer->SetText(FText::AsNumber(TimeRemaining, &Opts));
			TXT_Timer->SetColorAndOpacity(FSlateColor(TargetColor));
		}

		// 2. 도넛 프로그레스 바 업데이트 및 색상 설정
		if (RadialMaterial && TotalDuration > 0.0f)
		{
			float Percent = 1.0f - (TimeRemaining / TotalDuration);
			RadialMaterial->SetScalarParameterValue(FName("Percent"), Percent);
			RadialMaterial->SetVectorParameterValue(FName("Color"), TargetColor);
		}

		if (TimeRemaining <= 0.0f)
		{
			bIsCasting = false;
			SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}