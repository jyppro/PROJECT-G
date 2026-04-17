#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerStatusWidget.generated.h"

class UProgressBar;
class UTextBlock;

UCLASS()
class GUN_PHIRIA_API UPlayerStatusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 각 장비 및 상태의 퍼센트(0.0 ~ 1.0)를 업데이트하는 함수들
	void UpdateBackpackCapacity(float Percent);
	void UpdateHelmetDurability(float Percent);
	void UpdateVestDurability(float Percent);
	void UpdateAmmo(int32 CurrentAmmo, int32 ReserveAmmo);

	// 부스트 효과 업데이트 (지속시간 기반)
	void UpdateSpeedBoost(float Percent);
	void UpdateHealBoost(float Percent);
	void UpdateHealth(float Percent);

protected:
	// UMG의 위젯 이름과 반드시 동일해야 합니다.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PB_Backpack;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PB_Helmet;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PB_Vest;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TXT_CurrentAmmo;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TXT_BackpackAmmo;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PB_SpeedBoost;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PB_HealBoost;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PB_Health;
};