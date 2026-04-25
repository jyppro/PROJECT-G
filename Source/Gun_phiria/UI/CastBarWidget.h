#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CastBarWidget.generated.h"

class UImage;
class UTextBlock;
class UTexture2D;
class UMaterialInstanceDynamic;

UCLASS()
class GUN_PHIRIA_API UCastBarWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
	// 캐릭터가 시전을 시작할 때 C++에서 직접 호출할 함수
	void StartCast(float Duration, UTexture2D* ItemIcon, bool bInUseWarningColor = false, float InWarningTimeThreshold = 2.0f);

	UFUNCTION(BlueprintCallable, Category = "Casting")
	void StopCast();

protected:
	// [중요] 블루프린트 디자이너에 있는 위젯 이름과 정확히 일치해야 자동 연결됩니다!
	UPROPERTY(meta = (BindWidget))
	UImage* IMG_RadialBar;

	UPROPERTY(meta = (BindWidget))
	UImage* IMG_ItemIcon;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TXT_Timer;

	// 경고 색상 사용 여부 및 기준 시간
	bool bUseWarningColor = false;
	float WarningTimeThreshold = 2.0f;

private:
	float TotalDuration;
	float TimeRemaining;
	bool bIsCasting;

	// 머티리얼 파라미터를 조절하기 위한 다이내믹 머티리얼 포인터
	UPROPERTY()
	UMaterialInstanceDynamic* RadialMaterial;
};