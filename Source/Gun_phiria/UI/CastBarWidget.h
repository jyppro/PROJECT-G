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
	void StartCast(float Duration, UTexture2D* ItemIcon, bool bInUseWarningColor = false, float InWarningTimeThreshold = 2.0f);

	UFUNCTION(BlueprintCallable, Category = "Casting")
	void StopCast();

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> IMG_RadialBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> IMG_ItemIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TXT_Timer;

	bool bUseWarningColor = false;
	float WarningTimeThreshold = 2.0f;

private:
	float TotalDuration;
	float TimeRemaining;
	bool bIsCasting;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> RadialMaterial;

	// 중복되는 텍스트 및 색상 업데이트 로직을 묶어둔 헬퍼 함수
	void UpdateTimerUI(float CurrentTime, const FLinearColor& CurrentColor);
};