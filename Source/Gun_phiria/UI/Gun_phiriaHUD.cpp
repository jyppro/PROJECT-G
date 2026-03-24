#include "Gun_phiriaHUD.h"
#include "../Gun_phiriaCharacter.h"
#include "../Weapon/WeaponBase.h"

// Engine Headers
#include "Engine/Canvas.h"
#include "Kismet/GameplayStatics.h"

void AGun_phiriaHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas) return;

	const FVector2D Center(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f);
	TObjectPtr<AGun_phiriaCharacter> PlayerChar = Cast<AGun_phiriaCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));

	if (!PlayerChar) return;

	// 1. Crosshair Logic
	if (TObjectPtr<AWeaponBase> CurrentWeapon = PlayerChar->GetCurrentWeapon())
	{
		DrawCrosshair(PlayerChar, CurrentWeapon, Center);
	}

	// 2. Health Bar Logic
	DrawHealthBar(PlayerChar, Center);

	// 3. Mission Clear Message
	if (bIsShowingClearMessage)
	{
		DrawMissionClear(Center);
	}
}

void AGun_phiriaHUD::DrawCrosshair(TObjectPtr<AGun_phiriaCharacter> PlayerChar, TObjectPtr<AWeaponBase> CurrentWeapon, const FVector2D& Center)
{
	float CurrentFOV = 90.0f;
	if (TObjectPtr<APlayerController> PC = GetOwningPlayerController())
	{
		if (PC->PlayerCameraManager) CurrentFOV = PC->PlayerCameraManager->GetFOVAngle();
	}

	// Calculate Spread Offset
	const float SpreadAngle = PlayerChar->GetCurrentSpread() * CurrentWeapon->WeaponSpreadMultiplier;
	const float HalfFOVRadian = FMath::DegreesToRadians(CurrentFOV * 0.5f);
	const float SpreadRadian = FMath::DegreesToRadians(SpreadAngle);
	const float SpreadOffset = Center.X * (FMath::Tan(SpreadRadian) / FMath::Tan(HalfFOVRadian));
	const float FinalOffset = CurrentWeapon->WeaponBaseSpreadHUD + SpreadOffset;

	// Visual Settings
	FLinearColor CrosshairColor = FLinearColor::Black;
	float DotSize = 2.0f;

	if (PlayerChar->bIsAimingAtHead)
	{
		CrosshairColor = FLinearColor::Red;
		DotSize = 10.0f;
	}

	// Draw Center Dot
	const float HalfDot = DotSize * 0.5f;
	DrawRect(CrosshairColor, Center.X - HalfDot, Center.Y - HalfDot, DotSize, DotSize);

	// Draw Crosshair Lines (Only when not aiming)
	if (!PlayerChar->GetIsAiming())
	{
		const float HalfThick = CrosshairThickness * 0.5f;
		// Top, Bottom, Left, Right
		DrawRect(CrosshairColor, Center.X - HalfThick, Center.Y - FinalOffset - CrosshairLength, CrosshairThickness, CrosshairLength);
		DrawRect(CrosshairColor, Center.X - HalfThick, Center.Y + FinalOffset, CrosshairThickness, CrosshairLength);
		DrawRect(CrosshairColor, Center.X - FinalOffset - CrosshairLength, Center.Y - HalfThick, CrosshairLength, CrosshairThickness);
		DrawRect(CrosshairColor, Center.X + FinalOffset, Center.Y - HalfThick, CrosshairLength, CrosshairThickness);
	}
}

void AGun_phiriaHUD::DrawHealthBar(TObjectPtr<AGun_phiriaCharacter> PlayerChar, const FVector2D& Center)
{
	const float BarWidth = 400.0f;
	const float BarHeight = 15.0f;
	const float BarPosX = Center.X - (BarWidth * 0.5f);
	const float BarPosY = Canvas->ClipY - 60.0f;

	// Background
	DrawRect(FLinearColor(0.05f, 0.05f, 0.05f, 0.7f), BarPosX, BarPosY, BarWidth, BarHeight);

	// Foreground
	const float HealthPercent = FMath::Clamp(PlayerChar->CurrentHealth / PlayerChar->MaxHealth, 0.0f, 1.0f);
	FLinearColor HealthColor = (HealthPercent <= 0.3f) ? FLinearColor(0.8f, 0.0f, 0.0f, 1.0f) : FLinearColor::White;

	DrawRect(HealthColor, BarPosX, BarPosY, BarWidth * HealthPercent, BarHeight);
}

void AGun_phiriaHUD::DrawMissionClear(const FVector2D& Center)
{
	ClearMessageTimer -= GetWorld()->GetDeltaSeconds();

	if (ClearMessageTimer <= 0.0f)
	{
		bIsShowingClearMessage = false;
		return;
	}

	const float BoxWidth = 800.0f;
	const float BoxHeight = 150.0f;
	const float BoxPosX = Center.X - (BoxWidth * 0.5f);
	const float BoxPosY = Center.Y - 250.0f;

	// Draw Background Box
	DrawRect(FLinearColor(0.0f, 0.1f, 0.4f, 0.7f), BoxPosX, BoxPosY, BoxWidth, BoxHeight);

	// Draw Text
	DrawText(TEXT("MISSION CLEAR!"), FLinearColor::Yellow, BoxPosX + 150.0f, BoxPosY + 40.0f, nullptr, 4.0f);
}

void AGun_phiriaHUD::ShowMissionClearMessage()
{
	bIsShowingClearMessage = true;
	ClearMessageTimer = 3.0f;
}