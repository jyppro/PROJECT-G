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

	FLinearColor CrosshairColor = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);

	// ... (이하 DrawRect로 크로스헤어 선 4개 그리는 코드는 기존과 완벽히 동일하게 유지) ...
	DrawRect(CrosshairColor, Center.X - 1, Center.Y - 1, 2, 2);
	DrawRect(CrosshairColor, Center.X - (CrosshairThickness * 0.5f), Center.Y - FinalOffset - CrosshairLength, CrosshairThickness, CrosshairLength);
	DrawRect(CrosshairColor, Center.X - (CrosshairThickness * 0.5f), Center.Y + FinalOffset, CrosshairThickness, CrosshairLength);
	DrawRect(CrosshairColor, Center.X - FinalOffset - CrosshairLength, Center.Y - (CrosshairThickness * 0.5f), CrosshairLength, CrosshairThickness);
	DrawRect(CrosshairColor, Center.X + FinalOffset, Center.Y - (CrosshairThickness * 0.5f), CrosshairLength, CrosshairThickness);
}