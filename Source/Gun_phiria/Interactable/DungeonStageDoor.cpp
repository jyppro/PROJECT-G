#include "DungeonStageDoor.h"
#include "../Gun_phiriaCharacter.h" // 캐릭터 헤더 포함 (경로 주의)
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"
#include "../Gun_phiriaGameInstance.h"

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
	if (InteractingPlayer)
	{
		if (APlayerController* PC = Cast<APlayerController>(InteractingPlayer->GetController()))
		{
			bIsTransitioning = true;
			InteractingPlayer->DisableInput(PC);

			// ==========================================
			// [데이터 저장 실행!]
			// ==========================================
			UGun_phiriaGameInstance* GameInst = Cast<UGun_phiriaGameInstance>(GetGameInstance());
			if (GameInst)
			{
				// 문 타입이 ReturnVillage라면 true를 넘겨 사파이어만 저장
				bool bOnlySapphire = (DoorType == EDungeonDoorType::Door_ReturnVillage);
				GameInst->SavePlayerData(InteractingPlayer, bOnlySapphire);
			}

			if (PC->PlayerCameraManager)
			{
				PC->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, FadeOutDuration, FLinearColor::Black, false, true);
			}

			GetWorld()->GetTimerManager().SetTimer(LevelTransitionTimerHandle, this, &ADungeonStageDoor::OpenNextLevel, FadeOutDuration, false);
		}
	}
}

FString ADungeonStageDoor::GetInteractText_Implementation()
{
	// 에디터에서 설정한 문의 타입에 따라 텍스트를 다르게 띄워줍니다.
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

	// 만약 에디터에서 이동할 레벨 이름을 비워두었다면?
	// -> "현재 맵"의 이름을 가져와서 다시 로드합니다! (새로운 랜덤 던전 생성됨)
	if (LevelToLoad.IsNone())
	{
		LevelToLoad = FName(*GetWorld()->GetName());
	}

	UGameplayStatics::OpenLevel(this, LevelToLoad);
}