#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InventoryStudio.generated.h"

UCLASS()
class GUN_PHIRIA_API AInventoryStudio : public AActor
{
	GENERATED_BODY()

public:
	AInventoryStudio();

	// 외부(플레이어)에서 장비를 업데이트하라고 명령할 때 부르는 함수
	void UpdateStudioEquipment(class UStaticMesh* NewHelmet, class UStaticMesh* NewVest, class UStaticMesh* NewBackpack, class UStaticMesh* NewWeapon);

	// 인벤토리를 열 때 수동으로 캡처(찰칵!)하게 만드는 함수
	void CaptureProfile();

	void SetCameraLive(bool bIsLive);

protected:
	virtual void BeginPlay() override;

	// --- 렌더링용 카메라 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class USceneCaptureComponent2D> StudioCamera;

	// --- 가짜 몸통 및 장비들 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class USkeletalMeshComponent> CloneBodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UStaticMeshComponent> CloneHelmetMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UStaticMeshComponent> CloneVestMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UStaticMeshComponent> CloneBackpackMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UStaticMeshComponent> CloneWeaponMesh;
};