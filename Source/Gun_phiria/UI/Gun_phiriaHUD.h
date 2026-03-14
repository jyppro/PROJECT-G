#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Gun_phiriaHUD.generated.h"

UCLASS()
class AGun_phiriaHUD : public AHUD
{
	GENERATED_BODY()

public:
	// 화면에 UI를 그릴 때 매 프레임 호출되는 함수입니다.
	virtual void DrawHUD() override;

private:
	// 크로스헤어 시각적 설정값 (원하는 대로 조절하세요!)
	float CrosshairSpreadMultiplier = 15.0f; // 퍼짐 수치 1당 선이 멀어지는 픽셀 거리
	float BaseSpread = 5.0f;                 // 기본 상태일 때 선과 중앙 점 사이의 최소 거리
	float CrosshairLength = 12.0f;           // 선의 길이
	float CrosshairThickness = 2.0f;         // 선의 두께
};