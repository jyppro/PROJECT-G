#include "MapNodeWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"
#include "../Gun_phiriaGameInstance.h"
#include "Kismet/GameplayStatics.h"

void UMapNodeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (NodeButton)
	{
		// 버튼 클릭 시 OnNodeButtonClicked 함수가 실행되도록 연결
		NodeButton->OnClicked.AddDynamic(this, &UMapNodeWidget::OnNodeButtonClicked);
	}
}

void UMapNodeWidget::SetupNode(int32 InNodeID, FName InIconType, bool bIsCurrentNode, bool bIsSelectable)
{
	NodeID = InNodeID;
	RoomIconType = InIconType;

	// [1] 플레이어 위치 아이콘 보이기/숨기기
	if (PlayerIcon)
	{
		PlayerIcon->SetVisibility(bIsCurrentNode ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}

	// [2] 상태에 따른 마우스 반응 및 UMG 스타일 적용 처리
	if (bIsCurrentNode)
	{
		// [현재 노드] UMG의 Disabled 스타일 발동 방지 (원본 유지) + 마우스 클릭만 무시
		NodeButton->SetIsEnabled(true);
		NodeButton->SetVisibility(ESlateVisibility::HitTestInvisible);
		NodeButton->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
	}
	else if (bIsSelectable)
	{
		// [다음 이동 가능한 노드] 완전한 활성화 상태 (원본 유지 + 클릭 가능)
		NodeButton->SetIsEnabled(true);
		NodeButton->SetVisibility(ESlateVisibility::Visible);
		NodeButton->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
	}
	else
	{
		// [아직 갈 수 없는 미래의 노드] 버튼을 비활성화하여 블루프린트의 Disabled 설정을 그대로 사용
		NodeButton->SetIsEnabled(false);
		NodeButton->SetVisibility(ESlateVisibility::Visible); // 가시성은 정상 유지

		// Disabled 상태일 때 색상이 겹치지 않도록 기본 흰색으로 둡니다.
		// (블루프린트의 Disabled > Tint 색상이나 이미지가 이를 덮어쓰게 됩니다.)
		NodeButton->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
	}
}

void UMapNodeWidget::OnNodeButtonClicked()
{
	UGun_phiriaGameInstance* GameInst = Cast<UGun_phiriaGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	if (!GameInst) return;

	if (GameInst->CurrentRunMap.Contains(NodeID))
	{
		FStageNode SelectedNode = GameInst->CurrentRunMap[NodeID];
		GameInst->NextStageData = SelectedNode.StageData;
		GameInst->CurrentNodeID = NodeID;

		if (APlayerController* PC = GetOwningPlayer())
		{
			// [수정됨] 마우스 숨기고 다시 게임 입력 모드로 복구
			PC->SetShowMouseCursor(false);
			PC->SetInputMode(FInputModeGameOnly());

			// 캐릭터 조작 다시 활성화 (새 레벨로 가지만 안전을 위해)
			if (APawn* PlayerPawn = PC->GetPawn())
			{
				PlayerPawn->EnableInput(PC);
			}
		}

		// 주의: UGameplayStatics::SetGamePaused(GetWorld(), false); 삭제됨!

		// 다음 맵으로 이동
		UGameplayStatics::OpenLevel(GetWorld(), TEXT("DungeonLevel_Procedural"));
	}
}