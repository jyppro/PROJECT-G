#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Interface/InteractInterface.h"
#include "PickupItemBase.generated.h"

UCLASS()
class GUN_PHIRIA_API APickupItemBase : public AActor, public IInteractInterface
{
	GENERATED_BODY()

public:
	APickupItemBase();

	// 이 아이템이 무엇인지(ID)와 몇 개인지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FName ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 Quantity;

protected:
	// 눈에 보이는 모델링
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* ItemMesh;

	// 상호작용 범위를 인식할 콜리전 (선택사항, 트레이스로만 할거면 없어도 무방)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USphereComponent* OverlapSphere;

public:
	// --- IInteractInterface 구현부 ---
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual bool ShouldPlayAnimation_Implementation() const override { return true; } // 줍는 애니메이션 재생
	virtual FString GetInteractText_Implementation() override;
};