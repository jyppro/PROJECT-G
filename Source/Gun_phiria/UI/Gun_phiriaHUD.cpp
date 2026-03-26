#include "Gun_phiriaHUD.h"
#include "../Gun_phiriaCharacter.h"
#include "../Weapon/WeaponBase.h"
#include "../Interface/InteractInterface.h"

// Engine Headers
#include "Engine/Canvas.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Actor.h"

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

	DrawInteractPrompt(PlayerChar, Center);
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

void AGun_phiriaHUD::DrawInteractPrompt(TObjectPtr<AGun_phiriaCharacter> PlayerChar, const FVector2D& Center)
{
	AActor* Target = PlayerChar->TargetInteractable.Get();

	if (Target)
	{
		// 1. 표시할 텍스트 가져오기
		FString DisplayName = Target->GetName(); // 기본값: 액터 이름

		// 만약 타겟이 상호작용 인터페이스를 가지고 있다면, 전용 텍스트를 가져옵니다.
		if (Target->Implements<UInteractInterface>())
		{
			DisplayName = IInteractInterface::Execute_GetInteractText(Target);
		}

		FString InteractText = FString::Printf(TEXT("[F] - %s"), *DisplayName);

		// 2. 폰트 및 텍스트 크기 설정
		// 엔진 기본 폰트를 가져와서 정확한 텍스트 픽셀 크기를 측정합니다.
		UFont* DrawFont = GEngine->GetMediumFont();
		float TextScale = 1.2f;
		float TextWidth = 0.0f;
		float TextHeight = 0.0f;

		// 문자열이 화면에서 차지할 가로(TextWidth)와 세로(TextHeight) 픽셀 길이를 알아냅니다.
		GetTextSize(InteractText, TextWidth, TextHeight, DrawFont, TextScale);

		// 3. 위치 계산 (가운데 정렬)
		// 글씨의 절반 길이만큼 왼쪽으로 이동시켜서 크로스헤어 정중앙에 글씨가 오도록 맞춥니다.
		float TextPosX = Center.X - (TextWidth * 0.5f);
		float TextPosY = Center.Y + 50.0f;

		// 상자 여백 (글씨 주변의 까만 공간 크기)
		float PaddingX = 15.0f;
		float PaddingY = 5.0f;

		// 4. 글씨 길이에 맞춰진 동적 배경 박스 그리기
		DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.5f),
			TextPosX - PaddingX,
			TextPosY - PaddingY,
			TextWidth + (PaddingX * 2.0f),
			TextHeight + (PaddingY * 2.0f));

		// 5. 텍스트 그리기
		DrawText(InteractText, FLinearColor::White, TextPosX, TextPosY, DrawFont, TextScale);
	}
}