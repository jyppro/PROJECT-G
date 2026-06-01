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
		// 버튼 클릭 시 OnNodeButtonClicked 함수가 실행되도록 연결
		NodeButton->OnClicked.AddDynamic(this, &UMapNodeWidget::OnNodeButtonClicked);
	}
}

void UMapNodeWidget::SetupNode(int32 InNodeID, FName InIconType, bool bIsCurrentNode, bool bIsSelectable)
{
	NodeID = InNodeID;
	RoomIconType = InIconType;

	// 1. 상태에 따른 버튼 색상 및 클릭 제어
	if (bIsCurrentNode)
	{
		NodeButton->SetIsEnabled(false);
		// 현재 위치한 노드는 버튼을 까맣게 칠합니다.
		NodeButton->SetColorAndOpacity(FLinearColor(0.15f, 0.15f, 0.15f, 1.0f));
	}
	else if (bIsSelectable)
	{
		NodeButton->SetIsEnabled(true);
		NodeButton->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
	}
	else
	{
		NodeButton->SetIsEnabled(false);
		// 갈 수 없는 노드는 안개에 가려진 것처럼 적당히 어둡고 반투명하게 처리
		NodeButton->SetColorAndOpacity(FLinearColor(0.4f, 0.4f, 0.4f, 0.8f));
	}

	// 2. RoomIcon 설정 및 0번 노드 아이콘 숨기기
	if (RoomIcon)
	{
		if (InNodeID == 0) // 첫 번째 시작 스테이지 (0층)
		{
			// 시작 노드는 아이콘을 숨깁니다.
			RoomIcon->SetVisibility(ESlateVisibility::Hidden);
		}
		else
		{
			// 나머지 노드는 아이콘을 보여줍니다.
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
			// 플레이어 아이콘은 항상 선명한 100% 불투명도를 유지하도록 강제 설정
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
	UGun_phiriaGameInstance* GameInst = Cast<UGun_phiriaGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	if (!GameInst) return;

	// 1. 선택한 노드의 데이터를 GameInstance에 업데이트
	if (GameInst->CurrentRunMap.Contains(NodeID))
	{
		FStageNode SelectedNode = GameInst->CurrentRunMap[NodeID];

		// 방금 선택한 노드의 데이터를 '다음 생성할 맵 데이터'로 확정!
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
				// 무기, 탄약, 체력 등 현재 상태를 저장 (다음 맵에서 그대로 로드됨)
				GameInst->SavePlayerData(PlayerChar, false);
			}
		}

		// 3. [핵심] 다른 레벨로 가는 것이 아니라, '현재 레벨'을 재시작(Reload) 합니다.
		// 레벨이 다시 열리면서 기존의 방들은 깔끔하게 메모리에서 날아가고, 
		// ADungeonGenerator가 방금 넣은 새로운 NextStageData를 읽어 맵을 처음부터 다시 생성합니다!
		FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(GetWorld());
		UGameplayStatics::OpenLevel(GetWorld(), FName(*CurrentLevelName));
	}
}