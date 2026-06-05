#include "AnvilInteractable.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "../UI/AnvilScreenWidget.h"
#include "../Reward/RewardData.h" // 1단계에서 만든 데이터 구조체 포함

AAnvilInteractable::AAnvilInteractable()
{
	PrimaryActorTick.bCanEverTick = false;

	AnvilMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AnvilMesh"));
	RootComponent = AnvilMesh;
}

void AAnvilInteractable::BeginPlay()
{
	Super::BeginPlay();
}

FString AAnvilInteractable::GetInteractText_Implementation()
{
	// 이미 사용했다면 텍스트를 다르게 띄워줍니다.
	return bIsUsed ? TEXT("이미 사용한 모루입니다.") : TEXT("모루 사용하기 (F)");
}

void AAnvilInteractable::Interact_Implementation(AActor* Interactor)
{
	// 이미 사용했거나, 데이터 테이블이나 UI 클래스가 없으면 조기 종료
	if (bIsUsed || !AnvilRewardTable || !AnvilUIClass) return;

	// 1. 데이터 테이블의 모든 행(Row) 이름 가져오기
	TArray<FName> RowNames = AnvilRewardTable->GetRowNames();
	if (RowNames.Num() < 3)
	{
		UE_LOG(LogTemp, Warning, TEXT("모루 보상 데이터 테이블에 데이터가 3개 미만입니다!"));
		return;
	}

	// 2. 피셔-예이츠 셔플(Fisher-Yates Shuffle)로 배열 무작위 섞기
	for (int32 i = RowNames.Num() - 1; i > 0; i--)
	{
		int32 RandomIndex = FMath::RandRange(0, i);
		RowNames.Swap(i, RandomIndex);
	}

	// 3. 앞에서부터 딱 3개만 추출하여 데이터 담기
	TArray<FAnvilRewardData*> SelectedRewards;
	for (int32 i = 0; i < 3; i++)
	{
		FAnvilRewardData* RowData = AnvilRewardTable->FindRow<FAnvilRewardData>(RowNames[i], TEXT("AnvilRewardExtraction"));
		if (RowData)
		{
			SelectedRewards.Add(RowData);
		}
	}

	// 4. 모루 UI 화면에 띄우기 및 조작 권한 뺏기
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC)
	{
		UAnvilScreenWidget* AnvilWidget = CreateWidget<UAnvilScreenWidget>(PC, AnvilUIClass); // [수정] 캐스팅을 위해 클래스 명시
		if (AnvilWidget)
		{
			// [해결] 1단계: 추출한 3개의 데이터를 UI로 전달
			AnvilWidget->SetupAnvilUI(SelectedRewards);

			AnvilWidget->AddToViewport();

			// 게임을 일시정지시키고 입력 모드를 UI 전용으로 변경
			UGameplayStatics::SetGamePaused(GetWorld(), true);

			// [해결] 2단계: TakeWidget() 삭제 (UI 증발 버그 방지)
			FInputModeUIOnly InputMode;
			PC->SetInputMode(InputMode);
			PC->SetShowMouseCursor(true);

			// 사용 완료 처리
			bIsUsed = true;
		}
	}
}