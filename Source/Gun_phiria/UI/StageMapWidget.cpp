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

	MapCanvas->ClearChildren();
	SpawnedNodeWidgets.Empty();

	// 맵 배치 설정값
	const float BaseY = 2000.0f;      // 0층(모루)의 Y좌표
	const float FloorHeight = 350.0f; // 층간 높이 간격
	const float ColumnWidth = 300.0f; // 가로 간격
	const float CenterX = 0.0f;       // 캔버스 중앙 X좌표

	for (const auto& Pair : GameInst->CurrentRunMap)
	{
		const FStageNode& NodeData = Pair.Value;

		UMapNodeWidget* NewNodeWidget = CreateWidget<UMapNodeWidget>(this, MapNodeWidgetClass);
		if (NewNodeWidget)
		{
			NewNodeWidget->SetupNode(NodeData.NodeID, NodeData.RoomIconType);

			UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(MapCanvas->AddChild(NewNodeWidget));
			if (CanvasSlot)
			{
				CanvasSlot->SetAutoSize(true);
				CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f)); // 정중앙 앵커 기준

				// Y좌표 계산
				float TargetY = BaseY - (NodeData.FloorLevel * FloorHeight);

				// X좌표 계산 (1개일 땐 중앙, 3개일 땐 좌/중/우 배치)
				float TargetX = CenterX;
				if (NodeData.FloorLevel != 0 && NodeData.FloorLevel != 5)
				{
					TargetX = CenterX + ((NodeData.ColumnIndex - 1) * ColumnWidth);
				}

				CanvasSlot->SetPosition(FVector2D(TargetX, TargetY));
			}

			SpawnedNodeWidgets.Add(NodeData.NodeID, NewNodeWidget);
		}
	}
}

// [추가] 선을 그릴 시작점과 끝점 좌표 배열을 생성하여 블루프린트로 넘겨줌
TArray<FMapLineData> UStageMapWidget::GetLineDataToDraw()
{
	TArray<FMapLineData> LineDataArray;

	UGun_phiriaGameInstance* GameInst = Cast<UGun_phiriaGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	if (!GameInst || !MapCanvas) return LineDataArray;

	for (const auto& Pair : GameInst->CurrentRunMap)
	{
		int32 CurrentNodeID = Pair.Key;
		const FStageNode& NodeInfo = Pair.Value;

		if (!SpawnedNodeWidgets.Contains(CurrentNodeID)) continue;

		UMapNodeWidget* StartWidget = SpawnedNodeWidgets[CurrentNodeID];
		UCanvasPanelSlot* StartSlot = Cast<UCanvasPanelSlot>(StartWidget->Slot);
		if (!StartSlot) continue;

		FVector2D StartPos = StartSlot->GetPosition();

		// 이 노드와 연결된 '다음 노드'들의 좌표를 찾아서 선 데이터 생성
		for (int32 NextNodeID : NodeInfo.ConnectedNextNodes)
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
					LineDataArray.Add(NewLine);
				}
			}
		}
	}

	return LineDataArray;
}