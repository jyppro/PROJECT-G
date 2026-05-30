#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MapNodeWidget.generated.h"

class UButton;
class UImage;

UCLASS()
class GUN_PHIRIA_API UMapNodeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 위젯 생성 시 호출하여 노드 정보를 주입하는 함수
	void SetupNode(int32 InNodeID, FName InIconType);

	// 현재 노드의 ID를 외부에서 참조할 수 있도록 Getter 제공
	int32 GetNodeID() const { return NodeID; }

protected:
	virtual void NativeConstruct() override;

	// 버튼 클릭 이벤트 바인딩용 함수
	UFUNCTION()
	void OnNodeButtonClicked();

	// ----------------------------------------------------
	// 블루프린트와 연동될 위젯들 (이름이 UMG 에디터의 위젯 이름과 정확히 일치해야 함)
	// ----------------------------------------------------
	UPROPERTY(meta = (BindWidget))
	UButton* NodeButton;

	UPROPERTY(meta = (BindWidget))
	UImage* RoomIcon;

private:
	int32 NodeID;
	FName RoomIconType;
};