#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Gun_phiriaHUD.generated.h"

UCLASS()
class GUN_PHIRIA_API AGun_phiriaHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	// 크로스헤어 시각적 설정값
	float BaseSpread = 5.0f;                 // 기본 상태일 때 선과 중앙 점 사이의 최소 거리
	float CrosshairLength = 12.0f;           // 선의 길이
	float CrosshairThickness = 2.0f;         // 선의 두께

	// 캐릭터 코드의 Fire()에서 사용한 스프레드 배수와 동일해야 합니다!
	float SpreadAngleMultiplier = 3.0f;
};