#include "Gun_phiriaHUD.h"
#include "../Gun_phiriaCharacter.h"
#include "../Weapon/WeaponBase.h"
#include "../Interface/InteractInterface.h"
#include "PlayerStatusWidget.h"
#include "../Component/InventoryComponent.h"
#include "Engine/Canvas.h"
#include "Kismet/GameplayStatics.h"

void AGun_phiriaHUD::BeginPlay()
{
	Super::BeginPlay();

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

	if (!PlayerChar->bIsCasting && !PlayerChar->bIsReloading)
	{
		if (TObjectPtr<AWeaponBase> CurrentWeapon = PlayerChar->GetCurrentWeapon())
		{
			DrawCrosshair(PlayerChar, CurrentWeapon, Center);
		}
	}

	if (bIsShowingClearMessage) DrawMissionClear(Center);

	DrawInteractPrompt(PlayerChar, Center);

	if (PlayerStatusWidget)
	{
		float HealthPercent = PlayerChar->MaxHealth > 0.0f ? (PlayerChar->CurrentHealth / PlayerChar->MaxHealth) : 0.0f;
		PlayerStatusWidget->UpdateHealth(HealthPercent);

		if (TObjectPtr<AWeaponBase> CurrentWeapon = PlayerChar->GetCurrentWeapon())
		{
			int32 ReserveAmmo = PlayerChar->PlayerInventory ? PlayerChar->PlayerInventory->GetTotalItemCount(CurrentWeapon->AmmoItemID) : 0;
			PlayerStatusWidget->UpdateAmmo(CurrentWeapon->CurrentAmmo, ReserveAmmo);
		}
		else
		{
			PlayerStatusWidget->UpdateAmmo(0, 0);
		}

		if (UInventoryComponent* Inv = PlayerChar->PlayerInventory)
		{
			float BackpackPercent = Inv->MaxWeight > 0.0f ? (Inv->CurrentWeight / Inv->MaxWeight) : 0.0f;
			PlayerStatusWidget->UpdateBackpackCapacity(BackpackPercent);

			float HelmetPercent = Inv->EquippedHelmetID.IsNone() ? 0.0f : FMath::Clamp(Inv->CurrentHelmetDurability / 100.0f, 0.0f, 1.0f);
			PlayerStatusWidget->UpdateHelmetDurability(HelmetPercent);

			float VestPercent = Inv->EquippedVestID.IsNone() ? 0.0f : FMath::Clamp(Inv->CurrentVestDurability / 100.0f, 0.0f, 1.0f);
			PlayerStatusWidget->UpdateVestDurability(VestPercent);
		}

		float SpeedBoostPercent = 0.0f;
		FTimerHandle SpeedTimer = PlayerChar->GetSpeedBuffTimerHandle();
		if (GetWorld()->GetTimerManager().IsTimerActive(SpeedTimer))
		{
			float Rate = GetWorld()->GetTimerManager().GetTimerRate(SpeedTimer);
			float Remaining = GetWorld()->GetTimerManager().GetTimerRemaining(SpeedTimer);
			SpeedBoostPercent = Rate > 0.0f ? (Remaining / Rate) : 0.0f;
		}
		PlayerStatusWidget->UpdateSpeedBoost(SpeedBoostPercent);

		float HealBoostPercent = 0.0f;
		int32 MaxHot = PlayerChar->GetMaxHOTTicks();
		int32 CurHot = PlayerChar->GetCurrentHOTTicks();
		if (MaxHot > 0 && CurHot > 0)
		{
			HealBoostPercent = FMath::Clamp(static_cast<float>(CurHot) / MaxHot, 0.0f, 1.0f);
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

	const float SpreadAngle = PlayerChar->GetCurrentSpread() * CurrentWeapon->WeaponSpreadMultiplier;
	const float HalfFOVRadian = FMath::DegreesToRadians(CurrentFOV * 0.5f);
	const float SpreadRadian = FMath::DegreesToRadians(SpreadAngle);
	const float SpreadOffset = Center.X * (FMath::Tan(SpreadRadian) / FMath::Tan(HalfFOVRadian));
	const float FinalOffset = CurrentWeapon->WeaponBaseSpreadHUD + SpreadOffset;

	FLinearColor CrosshairColor = PlayerChar->bIsAimingAtHead ? FLinearColor::Red : FLinearColor::Black;
	float DotSize = PlayerChar->bIsAimingAtHead ? 10.0f : 2.0f;
	const float HalfDot = DotSize * 0.5f;

	DrawRect(CrosshairColor, Center.X - HalfDot, Center.Y - HalfDot, DotSize, DotSize);

	if (!PlayerChar->GetIsAiming())
	{
		const float HalfThick = CrosshairThickness * 0.5f;
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

	DrawRect(FLinearColor(0.0f, 0.1f, 0.4f, 0.7f), BoxPosX, BoxPosY, BoxWidth, BoxHeight);
	DrawText(TEXT("MISSION CLEAR!"), FLinearColor::Yellow, BoxPosX + 150.0f, BoxPosY + 40.0f, nullptr, 4.0f);
}

void AGun_phiriaHUD::DrawInteractPrompt(TObjectPtr<AGun_phiriaCharacter> PlayerChar, const FVector2D& Center)
{
	AActor* Target = PlayerChar->TargetInteractable.Get();

	if (!Target) return;

	FString DisplayName = Target->GetName();
	if (Target->Implements<UInteractInterface>())
	{
		DisplayName = IInteractInterface::Execute_GetInteractText(Target);
	}

	FString InteractText = FString::Printf(TEXT("[F] %s"), *DisplayName);

	UFont* DrawFont = GEngine->GetMediumFont();
	float TextScale = 1.5f;
	float TextWidth = 0.0f;
	float TextHeight = 0.0f;

	GetTextSize(InteractText, TextWidth, TextHeight, DrawFont, TextScale);

	float TextPosX = Center.X + 180.0f;
	float TextPosY = Center.Y + 80.0f;
	float PaddingX = 12.0f;
	float PaddingY = 6.0f;

	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.4f), TextPosX - PaddingX, TextPosY - PaddingY, TextWidth + (PaddingX * 2.0f), TextHeight + (PaddingY * 2.0f));
	DrawText(InteractText, FLinearColor::White, TextPosX, TextPosY, DrawFont, TextScale);
}

void AGun_phiriaHUD::ShowMissionClearMessage()
{
	bIsShowingClearMessage = true;
	ClearMessageTimer = 3.0f;
}