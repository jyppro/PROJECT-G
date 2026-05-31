#include "DungeonStageDoor.h"
#include "../Gun_phiriaCharacter.h" 
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"
#include "../Gun_phiriaGameInstance.h"
#include "Blueprint/UserWidget.h" // [추가] 위젯 생성을 위한 헤더

ADungeonStageDoor::ADungeonStageDoor()
{
	PrimaryActorTick.bCanEverTick = false;

	DefaultRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultRoot"));
	RootComponent = DefaultRoot;

	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	DoorMesh->SetupAttachment(DefaultRoot);

	DoorMesh->SetCollisionProfileName(TEXT("BlockAll"));
	DoorMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	DoorType = EDungeonDoorType::Door_NextStage;
	NextLevelName = NAME_None;
	FadeOutDuration = 2.0f;
}

void ADungeonStageDoor::Interact_Implementation(AActor* Interactor)
{
	if (bIsTransitioning) return;

	InteractingPlayer = Cast<AGun_phiriaCharacter>(Interactor);
	if (!InteractingPlayer) return;

	APlayerController* PC = Cast<APlayerController>(InteractingPlayer->GetController());
	if (!PC) return;

	bIsTransitioning = true;
	InteractingPlayer->DisableInput(PC);

	// [디버그 1] 상호작용 성공
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, TEXT("1. Interact Start!"));

	if (UGun_phiriaGameInstance* GameInst = Cast<UGun_phiriaGameInstance>(GetGameInstance()))
	{
		bool bOnlySapphire = (DoorType == EDungeonDoorType::Door_ReturnVillage);
		GameInst->SavePlayerData(InteractingPlayer, bOnlySapphire);
	}

	if (DoorType == EDungeonDoorType::Door_NextStage)
	{
		// [디버그 2] 문 타입 통과
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, TEXT("2. Door Type is NextStage!"));

		if (StageMapWidgetClass)
		{
			// [디버그 3] 위젯 클래스 할당 확인
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, TEXT("3. Widget Class is Valid!"));

			UGun_phiriaGameInstance* GameInst = Cast<UGun_phiriaGameInstance>(GetGameInstance());
			if (GameInst && GameInst->CurrentRunMap.IsEmpty())
			{
				GameInst->GenerateRunMap();
				if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("3-1. Map Data Generated!"));
			}

			if (UUserWidget* MapUI = CreateWidget<UUserWidget>(PC, StageMapWidgetClass))
			{
				MapUI->AddToViewport(9999);

				if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("4. UI AddToViewport Success!!!"));

				// 마우스 커서 켜기
				PC->SetShowMouseCursor(true);

				// [수정됨] 특정 위젯을 강제로 포커스하지 않고, 모드만 UI 전용으로 변경
				FInputModeUIOnly InputMode;
				PC->SetInputMode(InputMode);

				return;
			}
			else
			{
				if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Error: CreateWidget Failed!"));
			}
		}
		else
		{
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Error: StageMapWidgetClass is NULL!"));
		}

		// 에러가 났을 경우 갇히지 않게 조작 복구
		InteractingPlayer->EnableInput(PC);
		bIsTransitioning = false;
		return;
	}

	// 이하 Return Village 로직 (생략)
}

FString ADungeonStageDoor::GetInteractText_Implementation()
{
	switch (DoorType)
	{
	case EDungeonDoorType::Door_NextStage:
		return TEXT("Next Stage");
	case EDungeonDoorType::Door_ReturnVillage:
		return TEXT("Dungeon Clear!");
	default:
		return TEXT("Open Door");
	}
}

void ADungeonStageDoor::OpenNextLevel()
{
	FName LevelToLoad = NextLevelName;

	if (LevelToLoad.IsNone())
	{
		LevelToLoad = FName(*GetWorld()->GetName());
	}

	UGameplayStatics::OpenLevel(this, LevelToLoad);
}