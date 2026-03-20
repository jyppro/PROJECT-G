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
}