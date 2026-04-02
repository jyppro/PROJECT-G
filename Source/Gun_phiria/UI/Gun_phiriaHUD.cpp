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
	if (!PlayerChar->bIsCasting)
	{
		if (TObjectPtr<AWeaponBase> CurrentWeapon = PlayerChar->GetCurrentWeapon())
		{
			DrawCrosshair(PlayerChar, CurrentWeapon, Center);
		}
	}

	// 2. Health Bar Logic
	DrawHealthBar(PlayerChar, Center);

	// 3. Mission Clear Message
	if (bIsShowingClearMessage)
	{
		DrawMissionClear(Center);
	}

	DrawInteractPrompt(PlayerChar, Center);
	DrawCurrency(PlayerChar);
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
		FString DisplayName = Target->GetName();
		if (Target->Implements<UInteractInterface>())
		{
			DisplayName = IInteractInterface::Execute_GetInteractText(Target);
		}

		// 배그 스타일 텍스트 조합 (예: "[F] 대용량 퀵 드로우 탄창 줍기")
		FString InteractText = FString::Printf(TEXT("[F] %s"), *DisplayName);

		// 2. 폰트 및 텍스트 크기 설정
		UFont* DrawFont = GEngine->GetMediumFont();
		float TextScale = 1.5f;
		float TextWidth = 0.0f;
		float TextHeight = 0.0f;

		GetTextSize(InteractText, TextWidth, TextHeight, DrawFont, TextScale);

		// 3. 고정 위치 계산 (화면 중앙 기준)
		float OffsetX = 180.0f; // 중앙에서 오른쪽으로 180픽셀 이동
		float OffsetY = 80.0f;  // 중앙에서 아래로 80픽셀 이동

		float TextPosX = Center.X + OffsetX;
		float TextPosY = Center.Y + OffsetY;

		// 4. 상자 여백 설정
		float PaddingX = 12.0f;
		float PaddingY = 6.0f;

		// 5. 배경 박스 그리기 (배그 느낌으로 투명도를 0.4 정도로 낮춤)
		DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.4f),
			TextPosX - PaddingX,
			TextPosY - PaddingY,
			TextWidth + (PaddingX * 2.0f),
			TextHeight + (PaddingY * 2.0f));

		// 6. 텍스트 그리기
		DrawText(InteractText, FLinearColor::White, TextPosX, TextPosY, DrawFont, TextScale);
	}
}

void AGun_phiriaHUD::DrawCurrency(TObjectPtr<AGun_phiriaCharacter> PlayerChar)
{
	// 1. 표시할 텍스트 문자열 만들기 (캐릭터의 재화 변수 접근)
	// 나중에 인벤토리 구조체가 생기면 이 부분의 참조 위치만 살짝 바꿔주면 돼!
	FString GoldText = FString::Printf(TEXT("Gold : %d"), PlayerChar->CurrentGold);
	FString SapphireText = FString::Printf(TEXT("Sapphire : %d"), PlayerChar->CurrentSapphire);

	// 2. 폰트 및 배율 설정
	UFont* DrawFont = GEngine->GetMediumFont();
	float TextScale = 1.5f; // 글씨가 잘 보이도록 조금 키웠어

	// 3. 텍스트를 그릴 화면 좌표 설정 (화면 우측 상단)
	// Canvas->ClipX는 화면의 전체 가로 길이입니다. 오른쪽 끝에서 조금 안쪽으로 당겨옵니다.
	float PosX = Canvas->ClipX - 250.0f;
	float PosY = 50.0f; // 위에서부터 50픽셀 아래

	// 4. 화면에 텍스트 그리기 (골드는 노란색, 사파이어는 청록색)
	// 그림자 효과를 주기 위해 검은색 텍스트를 살짝 어긋나게 먼저 그릴 수도 있지만, 직관성을 위해 바로 그립니다.
	DrawText(GoldText, FLinearColor::Yellow, PosX, PosY, DrawFont, TextScale);
	DrawText(SapphireText, FLinearColor(0.0f, 1.0f, 1.0f, 1.0f), PosX, PosY + 40.0f, DrawFont, TextScale);
}