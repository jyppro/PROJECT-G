#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemTooltipWidget.generated.h"

class UTextBlock;
class UDataTable;

UCLASS()
class GUN_PHIRIA_API UItemTooltipWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 데이터 테이블 정보를 받아 UI 텍스트를 업데이트하는 함수
	void UpdateTooltip(FName ItemID, UDataTable* ItemDataTable);

	UPROPERTY(BlueprintReadWrite, Category = "Inventory")
	bool bIsVicinitySlot = false;

protected:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* TXT_Name;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TXT_Description;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TXT_Type;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TXT_Capacity;

	UPROPERTY(meta = (BindWidget))
	class UImage* IMG_ItemIcon;
};