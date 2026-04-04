#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DragVisualWidget.generated.h"

UCLASS()
class GUN_PHIRIA_API UDragVisualWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 블루프린트의 IMG_DragIcon과 연결됩니다.
	UPROPERTY(meta = (BindWidget))
	class UImage* IMG_DragIcon;

	// 아이콘 이미지를 세팅해 줄 함수
	void SetDragIcon(class UTexture2D* IconTexture);
};