#include "StageMapWidget.h"
#include "MapNodeWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Kismet/GameplayStatics.h"

void UStageMapWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildMapUI(); // 시작 시 맵 구축
}

void UStageMapWidget::BuildMapUI()
{
	if (!MapNodeWidgetClass || !MapCanvas) return;

	UGun_phiriaGameInstance* GameInst = Cast<UGun_phiriaGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	if (!GameInst) return;

	// 기존 노드 안전하게 삭제
	for (auto& Pair : SpawnedNodeWidgets)
	{
		if (Pair.Value) Pair.Value->RemoveFromParent();
	}
	SpawnedNodeWidgets.Empty();

	// ===============================================
	// [위에서 아래로 내려가는 탑다운 맵 간격 설정]
	// ===============================================
	const float BaseY = 300.0f;       // 0층(시작점)의 Y 좌표를 캔버스 '위쪽'으로 설정
	const float FloorHeight = 400.0f; // 층과 층 사이의 위아래 간격
	const float ColumnWidth = 350.0f; // 좌우 노드 간격
	const float CenterX = 960.0f;     // 캔버스 가로 정중앙

	for (const auto& Pair : GameInst->CurrentRunMap)
	{
		const FStageNode& NodeData = Pair.Value;

		UMapNodeWidget* NewNodeWidget = CreateWidget<UMapNodeWidget>(this, MapNodeWidgetClass);
		if (NewNodeWidget)
		{
			bool bIsCurrentNode = (NodeData.NodeID == GameInst->CurrentNodeID);
			bool bIsSelectable = false;

			// [수정] 현재 노드(0번)와 선으로 연결된 다음 층의 노드들만 선택 가능하게 만듭니다.
			if (GameInst->CurrentRunMap.Contains(GameInst->CurrentNodeID))
			{
				bIsSelectable = GameInst->CurrentRunMap[GameInst->CurrentNodeID].ConnectedNextNodes.Contains(NodeData.NodeID);
			}

			// 완성된 상태(Current, Selectable)를 위젯에 넘겨줍니다. 
			// (작성하신 SetupNode 코드가 여기서 완벽하게 받아 처리합니다!)
			NewNodeWidget->SetupNode(NodeData.NodeID, NodeData.RoomIconType, bIsCurrentNode, bIsSelectable);

			UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(MapCanvas->AddChild(NewNodeWidget));
			if (CanvasSlot)
			{
				CanvasSlot->SetAutoSize(true);
				CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
				CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f));

				// [수정 핵심] Y좌표 계산: 층이 올라갈수록 화면 '아래'로 내려가야 하므로 + 기호를 사용합니다!
				float TargetY = BaseY + (NodeData.FloorLevel * FloorHeight);

				// X좌표 계산 (기존과 동일)
				float TargetX = CenterX;
				if (NodeData.FloorLevel != 0 && NodeData.FloorLevel != 4)
				{
					TargetX = CenterX + ((NodeData.ColumnIndex - 1) * ColumnWidth);
				}

				CanvasSlot->SetPosition(FVector2D(TargetX, TargetY));
			}

			SpawnedNodeWidgets.Add(NodeData.NodeID, NewNodeWidget);
		}
	}

	// 정상적으로 생성되었는지 좌측 상단 디버그로 확인!
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Successfully Spawned Nodes: %d"), SpawnedNodeWidgets.Num()));
}

TArray<FMapLineData> UStageMapWidget::GetLineDataToDraw()
{
	TArray<FMapLineData> LineDataArray;

	UGun_phiriaGameInstance* GameInst = Cast<UGun_phiriaGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	if (!GameInst || !MapCanvas) return LineDataArray;

	// [수정 핵심] 전체 맵을 순회하지 않고, 오직 '현재 노드(CurrentNodeID)'만 검사합니다.
	int32 CurrentNodeID = GameInst->CurrentNodeID;

	// 현재 맵 데이터에 CurrentNodeID가 존재하는지 확인
	if (!GameInst->CurrentRunMap.Contains(CurrentNodeID) || !SpawnedNodeWidgets.Contains(CurrentNodeID))
	{
		return LineDataArray;
	}

	// 현재 노드의 데이터와 위젯 정보 가져오기
	const FStageNode& CurrentNodeInfo = GameInst->CurrentRunMap[CurrentNodeID];
	UMapNodeWidget* StartWidget = SpawnedNodeWidgets[CurrentNodeID];
	UCanvasPanelSlot* StartSlot = Cast<UCanvasPanelSlot>(StartWidget->Slot);

	if (!StartSlot) return LineDataArray;

	// 선이 시작될 현재 노드의 좌표
	FVector2D StartPos = StartSlot->GetPosition();

	// 오직 현재 노드와 연결된 '다음 노드'들로만 선을 생성합니다.
	for (int32 NextNodeID : CurrentNodeInfo.ConnectedNextNodes)
	{
		if (SpawnedNodeWidgets.Contains(NextNodeID))
		{
			UMapNodeWidget* EndWidget = SpawnedNodeWidgets[NextNodeID];
			UCanvasPanelSlot* EndSlot = Cast<UCanvasPanelSlot>(EndWidget->Slot);

			if (EndSlot)
			{
				FMapLineData NewLine;
				NewLine.StartPos = StartPos;
				NewLine.EndPos = EndSlot->GetPosition();

				// 이 선들은 무조건 현재 노드에서 뻗어나가므로 항상 하얀색으로 칠합니다.
				NewLine.LineColor = FLinearColor::White;

				LineDataArray.Add(NewLine);
			}
		}
	}

	// 완성된 선 데이터 배열을 반환합니다.
	return LineDataArray;
}