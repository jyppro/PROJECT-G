#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Interface/InteractInterface.h"
#include "ShopDesk.generated.h"

UCLASS()
class GUN_PHIRIA_API AShopDesk : public AActor, public IInteractInterface
{
	GENERATED_BODY()

public:
	AShopDesk();

	// 상호작용 인터페이스 구현
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual FString GetInteractText_Implementation() override;

	// 에디터에서 이 책상과 연결될 상점 NPC를 직접 지정해줘야 합니다!
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Shop")
	class AShopNPC* LinkedNPC;

protected:
	// 1. 액터의 중심을 잡아줄 보이지 않는 루트 컴포넌트입니다.
	UPROPERTY(VisibleAnywhere, Category = "Components")
	class USceneComponent* RootScene;

	// 2. 조립된 블루프린트(방수포 상자)를 통째로 담아줄 자식 액터 컴포넌트입니다!
	UPROPERTY(VisibleAnywhere, Category = "Components")
	class UChildActorComponent* DeskBlueprintVisual;

	// 3. 플레이어의 화면 중앙(레이캐스트)이 닿았을 때 상호작용을 감지할 투명한 박스입니다.
	UPROPERTY(VisibleAnywhere, Category = "Components")
	class UBoxComponent* InteractCollision;
};