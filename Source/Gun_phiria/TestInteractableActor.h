#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/InteractInterface.h"
#include "TestInteractableActor.generated.h"

UCLASS()
class GUN_PHIRIA_API ATestInteractableActor : public AActor, public IInteractInterface
{
	GENERATED_BODY()

public:
	ATestInteractableActor();

protected:
	// 물체의 외형을 담당할 스태틱 메시 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* StaticMesh;

	/** IInteractInterface 구현 */
	// BlueprintNativeEvent로 만들었기 때문에, C++에서는 _Implementation을 붙여서 구현해.
	virtual void Interact_Implementation(AActor* Interactor) override;
};