#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Interface/InteractInterface.h" // 상호작용 인터페이스 헤더 포함
#include "InteractableDoor.generated.h"

UCLASS()
class GUN_PHIRIA_API AInteractableDoor : public AActor, public IInteractInterface // 인터페이스 상속 추가
{
	GENERATED_BODY()

public:
	// 액터의 기본값을 설정합니다.
	AInteractableDoor();

	// 인터페이스의 상호작용 함수를 오버라이드합니다.
	// (기존의 Interact(APlayerController*) 대신 인터페이스 규격을 따릅니다.)
	virtual void Interact_Implementation(AActor* Interactor) override;

	// 인터페이스 함수 오버라이드
	virtual FString GetInteractText_Implementation() override;

protected:
	// 문을 표현하고 캐릭터의 스캔에 감지될 컴포넌트입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UStaticMeshComponent> DoorMesh;

	// 실제로 레벨을 열어주는 내부 함수입니다.
	void OpenNextLevel();

	// 이동할 다음 레벨의 이름입니다. 에디터에서 설정할 수 있습니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Transition")
	FName NextLevelName;

	// 페이드 아웃에 걸리는 시간입니다. (기본값: 2초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Transition")
	float FadeOutDuration;

	// 화면에 표시될 상호작용 문구 (에디터에서 수정 가능)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FString InteractPromptText;

private:
	// 타이머를 관리하기 위한 핸들입니다.
	FTimerHandle LevelTransitionTimerHandle;

	bool bIsTransitioning = false;
};