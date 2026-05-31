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

	// [1] 플레이어 위치 아이콘 보이기/숨기기 (UMG에서 PlayerIcon이 변수로 잘 등록되어 있어야 보입니다!)
	if (PlayerIcon)
	{
		PlayerIcon->SetVisibility(bIsCurrentNode ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}

	// [2] 상태에 따른 마우스 반응 및 UMG 스타일 적용 처리
	if (bIsCurrentNode)
	{
		// [현재 노드] 마우스 클릭 무시 + 색상을 까맣게(RGB 0.15) 변경하여 시각적으로 막혔음을 표시!
		NodeButton->SetIsEnabled(true);
		NodeButton->SetVisibility(ESlateVisibility::HitTestInvisible);

		// 여기가 문제였습니다! 흰색(1.0f) 대신 까맣게(0.15f) 칠해줍니다.
		NodeButton->SetColorAndOpacity(FLinearColor(0.15f, 0.15f, 0.15f, 1.0f));
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
		NodeButton->SetVisibility(ESlateVisibility::Visible);

		NodeButton->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
	}
}

void UMapNodeWidget::OnNodeButtonClicked()
{
	UGun_phiriaGameInstance* GameInst = Cast<UGun_phiriaGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	if (!GameInst) return;

	// 1. 선택한 노드가 맵 데이터에 존재하는지 확인
	if (GameInst->CurrentRunMap.Contains(NodeID))
	{
		// 2. GameInstance에 다음 맵 데이터 및 현재 위치 업데이트
		FStageNode SelectedNode = GameInst->CurrentRunMap[NodeID];
		GameInst->NextStageData = SelectedNode.StageData;
		GameInst->CurrentNodeID = NodeID;

		// 3. 마우스 커서 숨기기 및 입력 모드 원래대로 복구
		if (APlayerController* PC = GetOwningPlayer())
		{
			PC->SetShowMouseCursor(false);
			PC->SetInputMode(FInputModeGameOnly());

			// (선택) 캐릭터 조작 다시 활성화
			if (APawn* PlayerPawn = PC->GetPawn())
			{
				PlayerPawn->EnableInput(PC);
			}
		}

		// 4. 다음 던전 레벨 열기 (맵 이름이 다르면 알맞게 수정해주세요!)
		UGameplayStatics::OpenLevel(GetWorld(), TEXT("DungeonLevel_Procedural"));
	}
}