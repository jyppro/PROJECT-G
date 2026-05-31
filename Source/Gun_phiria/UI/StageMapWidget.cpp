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
			// [수정] 현재 노드인지, 다음으로 갈 수 있는 노드인지 판별
			bool bIsCurrentNode = (NodeData.NodeID == GameInst->CurrentNodeID);

			// 현재 플레이어가 있는 노드의 'ConnectedNextNodes'에 포함되어 있다면 선택 가능!
			bool bIsSelectable = false;
			if (GameInst->CurrentRunMap.Contains(GameInst->CurrentNodeID))
			{
				bIsSelectable = GameInst->CurrentRunMap[GameInst->CurrentNodeID].ConnectedNextNodes.Contains(NodeData.NodeID);
			}

			// 파라미터를 추가하여 셋업 함수 호출
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
				if (NodeData.FloorLevel != 0 && NodeData.FloorLevel != 5)
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

	// [복구됨] 전체 노드를 순회하는 바깥쪽 루프
	for (const auto& Pair : GameInst->CurrentRunMap)
	{
		int32 CurrentNodeID = Pair.Key;
		const FStageNode& NodeInfo = Pair.Value;

		if (!SpawnedNodeWidgets.Contains(CurrentNodeID)) continue;

		UMapNodeWidget* StartWidget = SpawnedNodeWidgets[CurrentNodeID];
		UCanvasPanelSlot* StartSlot = Cast<UCanvasPanelSlot>(StartWidget->Slot);
		if (!StartSlot) continue;

		// 여기서 StartPos가 정의됩니다!
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

					// [추가됨] 플레이어 위치에 따른 선 색상 분기
					if (CurrentNodeID == GameInst->CurrentNodeID)
					{
						NewLine.LineColor = FLinearColor::White;
					}
					else
					{
						NewLine.LineColor = FLinearColor(0.1f, 0.1f, 0.1f, 0.3f);
					}
					LineDataArray.Add(NewLine);
				}
			}
		}
	}

	return LineDataArray;
}