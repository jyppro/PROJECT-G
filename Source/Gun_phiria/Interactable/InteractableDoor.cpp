#include "InteractableDoor.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h" // ACharacter 캐스팅을 위해 추가
#include "TimerManager.h"

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

// 상호작용 시 실행되는 함수 (인터페이스 구현)
void AInteractableDoor::Interact_Implementation(AActor* Interactor)
{
	// 1. 이미 페이드 아웃(레벨 이동)이 시작되었다면, 더 이상 코드를 실행하지 않고 바로 빠져나갑니다. (F키 연타 방지)
	if (bIsTransitioning)
	{
		return;
	}

	if (ACharacter* PlayerChar = Cast<ACharacter>(Interactor))
	{
		if (APlayerController* PC = Cast<APlayerController>(PlayerChar->GetController()))
		{
			// 2. 이동이 시작되었음을 표시하고, 플레이어의 모든 입력(이동, 시점 회전, 공격, 상호작용 등)을 차단합니다.
			bIsTransitioning = true;
			PlayerChar->DisableInput(PC);

			// 3. 카메라 매니저를 가져와서 페이드 아웃 효과를 시작합니다.
			APlayerCameraManager* CameraManager = PC->PlayerCameraManager;
			if (CameraManager)
			{
				CameraManager->StartCameraFade(0.0f, 1.0f, FadeOutDuration, FLinearColor::Black, false, true);
			}

			// 4. 페이드 아웃이 진행되는 동안 기다렸다가 레벨을 이동하도록 타이머를 설정합니다.
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