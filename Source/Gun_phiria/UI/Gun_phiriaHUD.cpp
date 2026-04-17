#include "Gun_phiriaHUD.h"
#include "../Gun_phiriaCharacter.h"
#include "../Weapon/WeaponBase.h"
#include "../Interface/InteractInterface.h"
#include "PlayerStatusWidget.h"
#include "../Component/InventoryComponent.h"

// Engine Headers
#include "Engine/Canvas.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Actor.h"

void AGun_phiriaHUD::BeginPlay()
{
	Super::BeginPlay();

	// 게임이 시작될 때 UMG 위젯을 생성하고 화면에 추가해 줘
	if (PlayerStatusWidgetClass)
	{
		PlayerStatusWidget = CreateWidget<UPlayerStatusWidget>(GetWorld(), PlayerStatusWidgetClass);
		if (PlayerStatusWidget)
		{
			PlayerStatusWidget->AddToViewport();
		}
	}
}

void AGun_phiriaHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas) return;

	const FVector2D Center(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f);
	TObjectPtr<AGun_phiriaCharacter> PlayerChar = Cast<AGun_phiriaCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));

	if (!PlayerChar) return;

	// 1. Crosshair Logic (기존 동일)
	if (!PlayerChar->bIsCasting && !PlayerChar->bIsReloading)
	{
		if (TObjectPtr<AWeaponBase> CurrentWeapon = PlayerChar->GetCurrentWeapon())
		{
			DrawCrosshair(PlayerChar, CurrentWeapon, Center);
		}
	}

	// 2. Mission Clear Message (기존 동일)
	if (bIsShowingClearMessage) DrawMissionClear(Center);

	// 3. Interact Prompt (기존 동일)
	DrawInteractPrompt(PlayerChar, Center);

	// 4. [신규] PlayerStatus UMG 위젯 실시간 데이터 업데이트!
	if (PlayerStatusWidget)
	{
		// 1. 체력 업데이트
		float HealthPercent = PlayerChar->MaxHealth > 0.0f ? (PlayerChar->CurrentHealth / PlayerChar->MaxHealth) : 0.0f;
		PlayerStatusWidget->UpdateHealth(HealthPercent);

		// 2. 탄약 업데이트
		if (TObjectPtr<AWeaponBase> CurrentWeapon = PlayerChar->GetCurrentWeapon())
		{
			int32 ReserveAmmo = 0;
			if (PlayerChar->PlayerInventory)
			{
				ReserveAmmo = PlayerChar->PlayerInventory->GetTotalItemCount(CurrentWeapon->AmmoItemID);
			}
			PlayerStatusWidget->UpdateAmmo(CurrentWeapon->CurrentAmmo, ReserveAmmo);
		}
		else
		{
			PlayerStatusWidget->UpdateAmmo(0, 0);
		}

		// ==========================================================
		// [신규] 배틀그라운드 장비 및 부스트 실시간 데이터 연결 (완성본)
		// ==========================================================

		if (PlayerChar->PlayerInventory)
		{
			// 3. 가방 용량 (CurrentWeight / MaxWeight)
			float BackpackPercent = PlayerChar->PlayerInventory->MaxWeight > 0.0f ?
				(PlayerChar->PlayerInventory->CurrentWeight / PlayerChar->PlayerInventory->MaxWeight) : 0.0f;
			PlayerStatusWidget->UpdateBackpackCapacity(BackpackPercent);

			// 4. 헬멧 내구도 (장착 중이지 않으면 0%)
			float HelmetPercent = 0.0f;
			if (!PlayerChar->PlayerInventory->EquippedHelmetID.IsNone())
			{
				// *참고: 현재 헤더에 MaxDurability가 InventoryComponent에 없어서 기본값 100.0f로 계산합니다.
				// 나중에 데이터 테이블에서 가져오게 수정하셔도 됩니다.
				float MaxHelmetDurability = 100.0f;
				HelmetPercent = FMath::Clamp(PlayerChar->PlayerInventory->CurrentHelmetDurability / MaxHelmetDurability, 0.0f, 1.0f);
			}
			PlayerStatusWidget->UpdateHelmetDurability(HelmetPercent);

			// 5. 조끼 내구도 (장착 중이지 않으면 0%)
			float VestPercent = 0.0f;
			if (!PlayerChar->PlayerInventory->EquippedVestID.IsNone())
			{
				float MaxVestDurability = 100.0f;
				VestPercent = FMath::Clamp(PlayerChar->PlayerInventory->CurrentVestDurability / MaxVestDurability, 0.0f, 1.0f);
			}
			PlayerStatusWidget->UpdateVestDurability(VestPercent);
		}

		// 6. 스피드 부스트 (Getter 사용)
		float SpeedBoostPercent = 0.0f;
		FTimerHandle SpeedTimer = PlayerChar->GetSpeedBuffTimerHandle(); // 함수로 가져옴

		if (GetWorld()->GetTimerManager().IsTimerActive(SpeedTimer))
		{
			float Rate = GetWorld()->GetTimerManager().GetTimerRate(SpeedTimer);
			float Remaining = GetWorld()->GetTimerManager().GetTimerRemaining(SpeedTimer);
			SpeedBoostPercent = Rate > 0.0f ? (Remaining / Rate) : 0.0f;
		}
		PlayerStatusWidget->UpdateSpeedBoost(SpeedBoostPercent);

		// 7. 힐 부스트 (Getter 사용)
		float HealBoostPercent = 0.0f;
		int32 MaxHot = PlayerChar->GetMaxHOTTicks(); // 함수로 가져옴
		int32 CurHot = PlayerChar->GetCurrentHOTTicks(); // 함수로 가져옴

		if (MaxHot > 0)
		{
			HealBoostPercent = FMath::Clamp((float)CurHot / (float)MaxHot, 0.0f, 1.0f);
		}
		PlayerStatusWidget->UpdateHealBoost(HealBoostPercent);
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