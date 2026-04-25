#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Gun_phiriaHUD.generated.h"

class AGun_phiriaCharacter;
class AWeaponBase;
class UPlayerStatusWidget;

UCLASS()
class GUN_PHIRIA_API AGun_phiriaHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
	virtual void BeginPlay() override;
	void ShowMissionClearMessage();

protected:
	void DrawCrosshair(TObjectPtr<AGun_phiriaCharacter> PlayerChar, TObjectPtr<AWeaponBase> CurrentWeapon, const FVector2D& Center);
	void DrawMissionClear(const FVector2D& Center);
	void DrawInteractPrompt(TObjectPtr<AGun_phiriaCharacter> PlayerChar, const FVector2D& Center);

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UPlayerStatusWidget> PlayerStatusWidgetClass;

	UPROPERTY()
	TObjectPtr<UPlayerStatusWidget> PlayerStatusWidget;

private:
	float CrosshairLength = 12.0f;
	float CrosshairThickness = 2.0f;

	bool bIsShowingClearMessage = false;
	float ClearMessageTimer = 0.0f;
};