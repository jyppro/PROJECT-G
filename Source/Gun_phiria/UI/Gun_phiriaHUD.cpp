#include "Gun_phiriaHUD.h"
#include "Engine/Canvas.h"
#include "Kismet/GameplayStatics.h"
#include "../Gun_phiriaCharacter.h"
#include "../Weapon/WeaponBase.h"

void AGun_phiriaHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas) return;
	FVector2D Center(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f);

	AGun_phiriaCharacter* PlayerChar = Cast<AGun_phiriaCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (!PlayerChar) return;

	// [새로 추가된 부분] 현재 장착된 무기를 가져옵니다. 무기가 없으면 그리지 않습니다.
	AWeaponBase* CurrentWeapon = PlayerChar->GetCurrentWeapon();
	if (!CurrentWeapon) return;

	// ==========================================

	float CurrentFOV = 90.0f;
	APlayerController* PC = GetOwningPlayerController();
	if (PC && PC->PlayerCameraManager)
	{
		CurrentFOV = PC->PlayerCameraManager->GetFOVAngle();
	}

	// 캐릭터 코드에서처럼 무기 고유의 배수를 가져와서 똑같이 곱해줍니다.
	float SpreadAngle = PlayerChar->GetCurrentSpread() * CurrentWeapon->WeaponSpreadMultiplier;

	float HalfFOVRadian = FMath::DegreesToRadians(CurrentFOV * 0.5f);
	float SpreadRadian = FMath::DegreesToRadians(SpreadAngle);

	float SpreadOffset = Center.X * (FMath::Tan(SpreadRadian) / FMath::Tan(HalfFOVRadian));

	// ==========================================

	// 무기 고유의 기본 크로스헤어 벌어짐 수치를 사용합니다.
	float FinalOffset = CurrentWeapon->WeaponBaseSpreadHUD + SpreadOffset;

	// 평상시 크로스헤어 색상
	FLinearColor CrosshairColor = FLinearColor::Black;
	float DotSize = 2.0f; // 기본 점 크기

	// 캐릭터가 적의 머리를 조준하고 있다면?
	if (PlayerChar->bIsAimingAtHead)
	{
		// 크로스헤어 "전체" 색상을 빨간색으로 바꿉니다!
		CrosshairColor = FLinearColor::Red;
		// 중앙 점의 크기도 확 키워줍니다.
		DotSize = 10.0f;
	}

	float HalfDot = DotSize * 0.5f;

	// 1. 중앙 점 그리기 (변경된 크기와 통합된 색상 적용)
	DrawRect(CrosshairColor, Center.X - HalfDot, Center.Y - HalfDot, DotSize, DotSize);

	// 2. 십자선의 4방향 선 그리기 (점과 동일하게 변경된 CrosshairColor 적용!)
	DrawRect(CrosshairColor, Center.X - (CrosshairThickness * 0.5f), Center.Y - FinalOffset - CrosshairLength, CrosshairThickness, CrosshairLength);
	DrawRect(CrosshairColor, Center.X - (CrosshairThickness * 0.5f), Center.Y + FinalOffset, CrosshairThickness, CrosshairLength);
	DrawRect(CrosshairColor, Center.X - FinalOffset - CrosshairLength, Center.Y - (CrosshairThickness * 0.5f), CrosshairLength, CrosshairThickness);
	DrawRect(CrosshairColor, Center.X + FinalOffset, Center.Y - (CrosshairThickness * 0.5f), CrosshairLength, CrosshairThickness);

	// ==========================================
	// ★ [새로 추가된 부분] 배틀그라운드 스타일 하단 체력바 그리기

	// 체력바 크기 및 위치 설정
	float BarWidth = 400.0f;  // 체력바 가로 길이
	float BarHeight = 15.0f;  // 체력바 세로 두께

	// 화면 하단 중앙 계산 (바닥에서 60픽셀 위로 띄움)
	float BarPosX = Center.X - (BarWidth * 0.5f);
	float BarPosY = Canvas->ClipY - 60.0f;

	// 1. 체력바 배경 (어두운 반투명 회색)
	FLinearColor BackgroundColor = FLinearColor(0.05f, 0.05f, 0.05f, 0.7f);
	DrawRect(BackgroundColor, BarPosX, BarPosY, BarWidth, BarHeight);

	// 2. 현재 체력 비율 계산 (0.0 ~ 1.0)
	float HealthPercent = PlayerChar->CurrentHealth / PlayerChar->MaxHealth;
	float CurrentBarWidth = BarWidth * HealthPercent;

	// 3. 체력바 전경색 (배그처럼 기본은 흰색)
	FLinearColor HealthColor = FLinearColor::White;

	// 체력이 30% 이하로 떨어지면 붉은색으로 변하게 하여 위기감 조성!
	if (HealthPercent <= 0.3f)
	{
		HealthColor = FLinearColor(0.8f, 0.0f, 0.0f, 1.0f); // 진한 빨강
	}

	// 4. 현재 체력만큼 흰색 바 그리기
	DrawRect(HealthColor, BarPosX, BarPosY, CurrentBarWidth, BarHeight);
}