#include "MapNodeWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"
#include "../Gun_phiriaGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "../Gun_phiriaCharacter.h"

void UMapNodeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (NodeButton)
	{
		NodeButton->OnClicked.AddDynamic(this, &UMapNodeWidget::OnNodeButtonClicked);
	}
}

void UMapNodeWidget::SetupNode(int32 InNodeID, FName InIconType, bool bIsCurrentNode, bool bIsSelectable)
{
	NodeID = InNodeID;
	RoomIconType = InIconType;

	// [추가] 이 노드가 선택 가능한지 여부를 클래스 변수에 저장해둡니다.
	bIsNodeClickable = bIsSelectable;

	// 1. 상태에 따른 버튼 색상 및 호버/클릭 제어
	if (bIsCurrentNode)
	{
		NodeButton->SetIsEnabled(false);
		NodeButton->SetColorAndOpacity(FLinearColor(0.15f, 0.15f, 0.15f, 1.0f));
		// 기본 상태 유지
		NodeButton->SetVisibility(ESlateVisibility::Visible);
	}
	else if (bIsSelectable)
	{
		NodeButton->SetIsEnabled(true);
		NodeButton->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
		// [추가] 선택 가능한 노드는 마우스 반응(호버, 클릭)을 정상적으로 켭니다.
		NodeButton->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		NodeButton->SetIsEnabled(true);
		NodeButton->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
		// [핵심 수정] 마우스 포인터가 닿아도 완전히 무시합니다 (호버 이펙트 X, 클릭 X)
		NodeButton->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	// 2. RoomIcon 설정 및 0번 노드 아이콘 숨기기
	if (RoomIcon)
	{
		if (InNodeID == 0) // 첫 번째 시작 스테이지 (0층)
		{
			RoomIcon->SetVisibility(ESlateVisibility::Hidden);
		}
		else
		{
			RoomIcon->SetVisibility(ESlateVisibility::Visible);

			if (IconDictionary.Contains(InIconType))
			{
				RoomIcon->SetBrushFromTexture(IconDictionary[InIconType]);
				RoomIcon->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
			}
		}
	}

	// 3. 플레이어 위치 아이콘 표시 (가장 나중에 그려져서 위를 덮음)
	if (PlayerIcon)
	{
		if (bIsCurrentNode)
		{
			PlayerIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
			PlayerIcon->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
		}
		else
		{
			PlayerIcon->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void UMapNodeWidget::OnNodeButtonClicked()
{
	// [핵심 추가] 갈 수 없는 노드(선택 불가능)를 클릭했다면 아무 일도 일어나지 않고 즉시 반환!
	if (!bIsNodeClickable) return;

	UGun_phiriaGameInstance* GameInst = Cast<UGun_phiriaGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	if (!GameInst) return;

	// 1. 선택한 노드의 데이터를 GameInstance에 업데이트
	if (GameInst->CurrentRunMap.Contains(NodeID))
	{
		FStageNode SelectedNode = GameInst->CurrentRunMap[NodeID];

		GameInst->NextStageData = SelectedNode.StageData;
		GameInst->CurrentNodeID = NodeID;

		// 2. UI 조작 종료 및 플레이어 권한/정보 저장
		if (APlayerController* PC = GetOwningPlayer())
		{
			PC->SetShowMouseCursor(false);
			PC->SetInputMode(FInputModeGameOnly());

			if (AGun_phiriaCharacter* PlayerChar = Cast<AGun_phiriaCharacter>(PC->GetPawn()))
			{
				PlayerChar->EnableInput(PC);
				GameInst->SavePlayerData(PlayerChar, false);
			}
		}

		UGameplayStatics::SetGamePaused(GetWorld(), false);

		// 3. 현재 레벨 리로드 (절차적 맵 재생성)
		FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(GetWorld());
		UGameplayStatics::OpenLevel(GetWorld(), FName(*CurrentLevelName));
	}
}