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

void UMapNodeWidget::SetupNode(int32 InNodeID, FName InIconType)
{
	NodeID = InNodeID;
	RoomIconType = InIconType;

	// TODO: RoomIconType(예: "Artifact", "Shop")에 따라 RoomIcon 이미지 텍스처 변경 로직
	// 예: DataAsset이나 DataTable에서 아이콘 텍스처를 찾아와 RoomIcon->SetBrushFromTexture() 호출
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