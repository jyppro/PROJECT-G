#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Gun_phiriaHUD.generated.h"

class AGun_phiriaCharacter;
class AWeaponBase;

UCLASS()
class GUN_PHIRIA_API AGun_phiriaHUD : public AHUD
{
	GENERATED_BODY()

public:
	// --- Lifecycle & API ---
	virtual void DrawHUD() override;
	void ShowMissionClearMessage();

protected:
	// --- Helper Draw Functions ---
	void DrawCrosshair(TObjectPtr<AGun_phiriaCharacter> PlayerChar, TObjectPtr<AWeaponBase> CurrentWeapon, const FVector2D& Center);
	void DrawHealthBar(TObjectPtr<AGun_phiriaCharacter> PlayerChar, const FVector2D& Center);
	void DrawMissionClear(const FVector2D& Center);

private:
	// --- Crosshair Settings ---
	float CrosshairLength = 12.0f;
	float CrosshairThickness = 2.0f;

	// --- State Flags & Timers ---
	bool bIsShowingClearMessage = false;
	float ClearMessageTimer = 0.0f;

};