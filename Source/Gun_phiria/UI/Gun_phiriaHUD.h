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
	virtual void BeginPlay() override;
	void ShowMissionClearMessage();

protected:
	// --- Helper Draw Functions ---
	void DrawCrosshair(TObjectPtr<AGun_phiriaCharacter> PlayerChar, TObjectPtr<AWeaponBase> CurrentWeapon, const FVector2D& Center);
	// void DrawHealthBar(TObjectPtr<AGun_phiriaCharacter> PlayerChar, const FVector2D& Center);
	void DrawMissionClear(const FVector2D& Center);
	void DrawInteractPrompt(TObjectPtr<class AGun_phiriaCharacter> PlayerChar, const FVector2D& Center);

	// 에디터에서 방금 만든 위젯 블루프린트를 넣어줄 클래스 변수
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UPlayerStatusWidget> PlayerStatusWidgetClass;

	// 화면에 띄워진 위젯을 관리할 포인터
	UPROPERTY()
	TObjectPtr<UPlayerStatusWidget> PlayerStatusWidget;

private:
	// --- Crosshair Settings ---
	float CrosshairLength = 12.0f;
	float CrosshairThickness = 2.0f;

	// --- State Flags & Timers ---
	bool bIsShowingClearMessage = false;
	float ClearMessageTimer = 0.0f;

};