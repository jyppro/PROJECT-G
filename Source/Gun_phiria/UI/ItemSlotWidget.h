#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Engine/DataTable.h" // 데이터 테이블을 사용하기 위해 추가
#include "ItemSlotWidget.generated.h"

// 전방 선언 (헤더 포함을 줄여 컴파일 속도 향상)
class UImage;
class UTextBlock;
class APickupItemBase;
class UDragVisualWidget;

UCLASS()
class GUN_PHIRIA_API UItemSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 인벤토리에서 아이템 정보를 세팅할 때 호출할 함수
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetItemInfo(FName InItemID, int32 InQuantity);

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TSubclassOf<UUserWidget> TooltipWidgetClass;

	// 이 슬롯이 바닥에 있는 아이템인지 체크하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ItemSlot")
	bool bIsVicinitySlot = false;

	// [추가된 부분] 바닥에 있는 실제 아이템 액터를 가리키는 포인터
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ItemSlot")
	APickupItemBase* TargetItemActor = nullptr;

	// [추가 1] 이 슬롯이 "장비 장착 슬롯"인지 체크하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ItemSlot")
	bool bIsEquipSlot = false;

	// 이 슬롯이 총기(주무기1, 주무기2, 권총 등)를 장착하는 슬롯인가?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ItemSlot|Combat")
	bool bIsWeaponSlot = false;

	// 이 슬롯이 총기 부착물(스코프, 탄창, 손잡이 등)을 장착하는 슬롯인가?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ItemSlot|Combat")
	bool bIsAttachmentSlot = false;

	// 만약 부착물 슬롯이라면, 어떤 무기(예: 1번 주무기, 2번 주무기)에 속하는가? 
	// (0 = 1번무기, 1 = 2번무기, 2 = 권총 등으로 약속해서 사용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ItemSlot|Combat", meta = (EditCondition = "bIsAttachmentSlot"))
	int32 ParentWeaponIndex = 0;

	// 어떤 부착물 부위인지 식별 (예: Muzzle(총구), Grip(손잡이), Mag(탄창), Scope(스코프))
	// 나중에 Enum으로 바꾸면 더 좋지만, 우선 FName으로 식별 가능하게 만듭니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ItemSlot|Combat", meta = (EditCondition = "bIsAttachmentSlot"))
	FName AttachmentType = NAME_None;

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// 마우스가 들어올 때
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// 마우스가 나갈 때 (이 줄이 빠져있었습니다!)
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	// [추가 2] 블루프린트의 OnDragDetected 노드를 대체할 함수
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ItemSlot")
	FName CurrentItemID;

	// [추가 3] 아이템의 수량을 기억할 변수 (SetItemInfo에서 받은 값을 저장)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ItemSlot")
	int32 CurrentQuantity = 0;

	// ==========================================
	// C++ UI 바인딩 (이름이 블루프린트 UI 이름과 완전히 똑같아야 합니다!)
	// ==========================================

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UImage* IMG_ItemIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	UImage* IMG_Background;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_ItemName;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_ItemQuantity;

	// ==========================================
	// 데이터 테이블 설정
	// ==========================================

	// 블루프린트 디테일 패널에서 사용할 데이터 테이블을 할당할 수 있도록 만듭니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory Data")
	UDataTable* ItemDataTable;

	UPROPERTY(EditDefaultsOnly, Category = "Drag And Drop")
	TSubclassOf<UDragVisualWidget> DragVisualClass;

	// [추가] 드래그가 어떤 드롭존에도 들어가지 못하고 취소되었을 때 호출되는 함수
	virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
};