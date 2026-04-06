#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Interface/InteractInterface.h"
#include "DungeonStageDoor.generated.h"

// 문의 역할을 구분하는 열거형
UENUM(BlueprintType)
enum class EDungeonDoorType : uint8
{
	Door_NextStage UMETA(DisplayName = "Next Stage"),
	Door_ReturnVillage UMETA(DisplayName = "Clear & Return Village")
};

UCLASS()
class GUN_PHIRIA_API ADungeonStageDoor : public AActor, public IInteractInterface
{
	GENERATED_BODY()

public:
	ADungeonStageDoor();

	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual FString GetInteractText_Implementation() override;

protected:
	// 이 문이 어떤 역할을 하는지 (다음 층 vs 마을 귀환)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Transition")
	EDungeonDoorType DoorType;

	// 이동할 다음 레벨(맵)의 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Transition")
	FName NextLevelName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Transition")
	float FadeOutDuration;

	void OpenNextLevel();

	// 기준점이 되어줄 빈 루트 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class USceneComponent> DefaultRoot;

	// 문을 표현할 메시
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UStaticMeshComponent> DoorMesh;

private:
	FTimerHandle LevelTransitionTimerHandle;
	bool bIsTransitioning = false;

	// 레벨 이동 시 플레이어 정보를 임시로 들고 있기 위한 포인터
	UPROPERTY()
	class AGun_phiriaCharacter* InteractingPlayer;
};