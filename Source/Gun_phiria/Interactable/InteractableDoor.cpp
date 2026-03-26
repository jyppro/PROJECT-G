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
	// 1. 상호작용한 액터(Interactor)가 캐릭터인지 확인하고 컨트롤러를 가져옵니다.
	if (ACharacter* PlayerChar = Cast<ACharacter>(Interactor))
	{
		if (APlayerController* PC = Cast<APlayerController>(PlayerChar->GetController()))
		{
			// 2. 카메라 매니저를 가져와서 페이드 아웃 효과를 시작합니다.
			APlayerCameraManager* CameraManager = PC->PlayerCameraManager;
			if (CameraManager)
			{
				// (시작 투명도, 끝 투명도, 지속 시간, 색상, 오디오 페이드 여부, 페이드 아웃 유지 여부)
				CameraManager->StartCameraFade(0.0f, 1.0f, FadeOutDuration, FLinearColor::Black, false, true);
			}

			// 3. 페이드 아웃이 진행되는 동안 기다렸다가 레벨을 이동하도록 타이머를 설정합니다.
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