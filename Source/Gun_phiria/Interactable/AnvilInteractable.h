#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Interface/InteractInterface.h"
#include "../Reward/RewardData.h"
#include "AnvilInteractable.generated.h"

UCLASS()
class GUN_PHIRIA_API AAnvilInteractable : public AActor, public IInteractInterface
{
	GENERATED_BODY()

public:
	AAnvilInteractable();

	// --- 상호작용 인터페이스 구현 ---
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual FString GetInteractText_Implementation() override;

protected:
	virtual void BeginPlay() override;

	// --- 모루 구성 요소 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anvil|Components")
	TObjectPtr<class UStaticMeshComponent> AnvilMesh;

	// 에디터에서 할당할 모루 보상 데이터 테이블 (FAnvilRewardData 구조체 사용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anvil|Data")
	TObjectPtr<class UDataTable> AnvilRewardTable;

	// 상호작용 시 화면에 띄울 모루 전용 UI 위젯 클래스
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anvil|UI")
	TSubclassOf<class UUserWidget> AnvilUIClass;

private:
	// 한 번 사용하면 다시 사용할 수 없도록 막는 변수
	bool bIsUsed = false;
};