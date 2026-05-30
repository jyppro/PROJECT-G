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

	// ==========================================
	// 1. 공통: 플레이어 데이터 저장
	// ==========================================
	UGun_phiriaGameInstance* GameInst = Cast<UGun_phiriaGameInstance>(GetGameInstance());
	if (GameInst)
	{
		bool bOnlySapphire = (DoorType == EDungeonDoorType::Door_ReturnVillage);
		GameInst->SavePlayerData(InteractingPlayer, bOnlySapphire);
	}

	// ==========================================
	// 2. 분기: 다음 스테이지(NextStage)일 경우 -> 맵 UI 띄우기
	// ==========================================
	if (DoorType == EDungeonDoorType::Door_NextStage)
	{
		if (StageMapWidgetClass)
		{
			// 맵 데이터가 비어있다면 생성
			if (GameInst && GameInst->CurrentRunMap.IsEmpty())
			{
				GameInst->GenerateRunMap();
			}

			if (UUserWidget* MapUI = CreateWidget<UUserWidget>(PC, StageMapWidgetClass))
			{
				MapUI->AddToViewport();

				// [수정됨] 마우스 활성화 및 UI 전용 입력 모드로 변경
				PC->SetShowMouseCursor(true);

				// 포커스를 UI로 맞추고 게임 조작(이동, 공격 등)을 차단
				FInputModeUIOnly InputMode;
				InputMode.SetWidgetToFocus(MapUI->TakeWidget());
				PC->SetInputMode(InputMode);

				// 주의: UGameplayStatics::SetGamePaused(GetWorld(), true); 삭제됨!

				return;
			}
		}
		else
		{
			// [안전장치] 만약 블루프린트에서 위젯을 할당하지 않았다면 화면에 빨간 경고 띄우기
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("[Error] StageMapWidgetClass is NULL! Please assign it in the Blueprint!"));

			// 에러가 났으므로 갇히지 않게 조작을 다시 풀어줌
			InteractingPlayer->EnableInput(PC);
			bIsTransitioning = false;
			return;
		}
	}

	// ==========================================
	// 3. 분기: 마을 귀환(ReturnVillage)이거나 위젯이 없을 경우 -> 페이드아웃 후 즉시 이동
	// ==========================================
	if (PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, FadeOutDuration, FLinearColor::Black, false, true);
	}

	GetWorld()->GetTimerManager().SetTimer(LevelTransitionTimerHandle, this, &ADungeonStageDoor::OpenNextLevel, FadeOutDuration, false);
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