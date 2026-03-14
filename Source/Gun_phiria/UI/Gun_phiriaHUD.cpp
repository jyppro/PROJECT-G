#include "Gun_phiriaHUD.h"
#include "Engine/Canvas.h"
#include "Gun_phiria\Gun_phiriaCharacter.h"
#include "Kismet/GameplayStatics.h"

void AGun_phiriaHUD::DrawHUD()
{
	Super::DrawHUD();

	// 캔버스가 없으면 그릴 수 없으므로 안전 장치 추가
	if (!Canvas) return;

	// 1. 화면의 정중앙 좌표 구하기
	FVector2D Center(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f);

	// 2. 플레이어 캐릭터 찾아오기
	AGun_phiriaCharacter* PlayerChar = Cast<AGun_phiriaCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (!PlayerChar) return;

	// 3. 퍼짐(Spread) 수치를 가져와서 픽셀 단위 오프셋으로 변환
	float SpreadOffset = PlayerChar->GetCurrentSpread() * CrosshairSpreadMultiplier;
	float FinalOffset = BaseSpread + SpreadOffset; // 최종적으로 중앙에서 떨어질 거리

	// 크로스헤어 색상
	FLinearColor CrosshairColor = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);

	// ==========================================
	// 4. 크로스헤어 그리기 (중앙 점 1개 + 주변 선 4개)
	// ==========================================

	// 중앙 점 (2x2 픽셀 크기)
	DrawRect(CrosshairColor, Center.X - 1, Center.Y - 1, 2, 2);

	// 위쪽 선 (Top)
	DrawRect(CrosshairColor,
		Center.X - (CrosshairThickness * 0.5f),
		Center.Y - FinalOffset - CrosshairLength,
		CrosshairThickness, CrosshairLength);

	// 아래쪽 선 (Bottom)
	DrawRect(CrosshairColor,
		Center.X - (CrosshairThickness * 0.5f),
		Center.Y + FinalOffset,
		CrosshairThickness, CrosshairLength);

	// 왼쪽 선 (Left)
	DrawRect(CrosshairColor,
		Center.X - FinalOffset - CrosshairLength,
		Center.Y - (CrosshairThickness * 0.5f),
		CrosshairLength, CrosshairThickness);

	// 오른쪽 선 (Right)
	DrawRect(CrosshairColor,
		Center.X + FinalOffset,
		Center.Y - (CrosshairThickness * 0.5f),
		CrosshairLength, CrosshairThickness);
}