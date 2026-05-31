#include "InteractableDoor.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h" // ACharacter 캐스팅을 위해 추가
#include "TimerManager.h"
#include "../Gun_phiriaGameInstance.h"

// 기본값을 설정합니다.
AInteractableDoor::AInteractableDoor()
{
	PrimaryActorTick.bCanEverTick = false;

	// 1. 스태틱 메시 컴포넌트 생성 및 루트로 설정
	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	RootComponent = DoorMesh;

	// 2. 캐릭터의 스캔 레이저(ECC_Visibility)에 블록되도록 설정하여 상호작용 감지가 가능하게 합니다.
	DoorMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	// 기본 변수 초기화
	NextLevelName = TEXT("NextLevelMap");
	FadeOutDuration = 2.0f; // 2초 동안 페이드 아웃

	InteractPromptText = TEXT("Enter Dungeon");
}

void AInteractableDoor::Interact_Implementation(AActor* Interactor)
{
	if (bIsTransitioning)
	{
		return;
	}

	if (ACharacter* PlayerChar = Cast<ACharacter>(Interactor))
	{
		if (APlayerController* PC = Cast<APlayerController>(PlayerChar->GetController()))
		{
			bIsTransitioning = true;
			PlayerChar->DisableInput(PC);

			// ▼▼▼ 이 부분이 확실히 들어있어야 합니다! ▼▼▼
			UGun_phiriaGameInstance* GameInst = Cast<UGun_phiriaGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
			if (GameInst)
			{
				GameInst->GenerateRunMap();
				GameInst->NextStageData = GameInst->BasicStartStageData;
			}

			APlayerCameraManager* CameraManager = PC->PlayerCameraManager;
			if (CameraManager)
			{
				CameraManager->StartCameraFade(0.0f, 1.0f, FadeOutDuration, FLinearColor::Black, false, true);
			}

			GetWorld()->GetTimerManager().SetTimer(
				LevelTransitionTimerHandle,
				this,
				&AInteractableDoor::OpenNextLevel,
				FadeOutDuration,
				false
			);
		}
	}
}

FString AInteractableDoor::GetInteractText_Implementation()
{
	return InteractPromptText;
}

// 다음 레벨을 엽니다.
void AInteractableDoor::OpenNextLevel()
{
	// 에디터에서 설정한 레벨 이름이 유효한지 확인 후 레벨 이동
	if (!NextLevelName.IsNone())
	{
		UGameplayStatics::OpenLevel(this, NextLevelName);
	}
}