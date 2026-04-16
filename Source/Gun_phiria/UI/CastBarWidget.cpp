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

void UCastBarWidget::StartCast(float Duration, UTexture2D* ItemIcon)
{
	TotalDuration = Duration;
	TimeRemaining = Duration;
	bIsCasting = true;

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

void UCastBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 시전 중일 때만 작동하도록 최적화
	if (bIsCasting && TimeRemaining > 0.0f)
	{
		TimeRemaining -= InDeltaTime;
		if (TimeRemaining < 0.0f) TimeRemaining = 0.0f;

		// 1. 텍스트 소수점 첫째 자리까지 실시간 업데이트
		if (TXT_Timer)
		{
			FNumberFormattingOptions Opts;
			Opts.MinimumFractionalDigits = 1;
			Opts.MaximumFractionalDigits = 1;
			TXT_Timer->SetText(FText::AsNumber(TimeRemaining, &Opts));
		}

		// 2. 도넛 프로그레스 바 실시간 업데이트
		if (RadialMaterial && TotalDuration > 0.0f)
		{
			float Percent = 1.0f - (TimeRemaining / TotalDuration);
			RadialMaterial->SetScalarParameterValue(FName("Percent"), Percent);
		}

		// 시전이 끝나면 틱 로직 정지
		if (TimeRemaining <= 0.0f)
		{
			bIsCasting = false;
		}
	}
}