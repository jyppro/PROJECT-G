#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../Gun_phiriaGameInstance.h"
#include "StageMapWidget.generated.h"

class UScrollBox;
class UCanvasPanel;
class UMapNodeWidget;

// [추가] 블루프린트에서 선을 그리기 위해 사용할 좌표 구조체
USTRUCT(BlueprintType)
struct FMapLineData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Map UI")
	FVector2D StartPos;

	UPROPERTY(BlueprintReadOnly, Category = "Map UI")
	FVector2D EndPos;

	UPROPERTY(BlueprintReadOnly)
	FLinearColor LineColor = FLinearColor::White;
};

UCLASS()
class GUN_PHIRIA_API UStageMapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 맵 UI 생성 및 노드 배치 함수
	UFUNCTION(BlueprintCallable, Category = "Map UI")
	void BuildMapUI();

	// [추가] 블루프린트 OnPaint에서 호출하여 그려야 할 선들의 좌표를 가져오는 함수
	UFUNCTION(BlueprintPure, Category = "Map UI")
	TArray<FMapLineData> GetLineDataToDraw();

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	UScrollBox* MapScrollBox;

	UPROPERTY(meta = (BindWidget))
	UCanvasPanel* MapCanvas;

	UPROPERTY(EditDefaultsOnly, Category = "Map UI")
	TSubclassOf<UMapNodeWidget> MapNodeWidgetClass;

private:
	// 생성된 노드 위젯들을 추적하기 위한 캐시 맵
	UPROPERTY()
	TMap<int32, UMapNodeWidget*> SpawnedNodeWidgets;
};