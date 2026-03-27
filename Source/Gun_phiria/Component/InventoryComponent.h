#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h" // 데이터 테이블 사용을 위해 필수
#include "InventoryComponent.generated.h"

// ========================================================
// 1. 아이템 데이터 구조체 (배그식 아이템 정보)
// ========================================================

UENUM(BlueprintType)
enum class EItemType : uint8
{
	Weapon			UMETA(DisplayName = "Weapon"),
	Attachment		UMETA(DisplayName = "Attachment"),
	Consumable		UMETA(DisplayName = "Consumable"),
	Throwable		UMETA(DisplayName = "Throwable"),
	Artifact		UMETA(DisplayName = "Artifact"),
	Misc			UMETA(DisplayName = "Misc")
};

USTRUCT(BlueprintType)
struct FItemData : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data")
	FName ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data")
	FText ItemName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data")
	FText ItemDescription;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data")
	TObjectPtr<class UTexture2D> ItemIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data")
	EItemType ItemType = EItemType::Misc;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data|Stats")
	float ItemWeight = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data")
	int32 MaxStackSize = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data|Economy")
	int32 BuyPrice = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data|Economy")
	int32 SellPrice = 0;
};

// ========================================================
// 2. 인벤토리 슬롯 (가방의 한 칸)
// ========================================================

USTRUCT(BlueprintType)
struct FInventorySlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Quantity = 0;

	// 빈 슬롯인지 확인하는 헬퍼 함수
	bool IsEmpty() const { return Quantity <= 0 || ItemID.IsNone(); }
};

// ========================================================
// 3. 인벤토리 컴포넌트 클래스
// ========================================================

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GUN_PHIRIA_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	// 아이템 획득 함수
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 AddItem(FName ItemID, int32 Quantity);

	// 아이템 버리기/사용 함수
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(FName ItemID, int32 Quantity);

	// 현재 가방의 상태 (슬롯 배열)
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Inventory")
	TArray<FInventorySlot> InventorySlots;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory|Weight")
	float CurrentWeight = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Weight")
	float MaxWeight = 100.0f; // 가방 레벨에 따라 나중에 올려줄 수 있음!

protected:
	virtual void BeginPlay() override;

	// 가방의 총 칸 수 (배그의 가방 용량 개념)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 MaxSlots = 20;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Data")
	class UDataTable* ItemDataTable;
};