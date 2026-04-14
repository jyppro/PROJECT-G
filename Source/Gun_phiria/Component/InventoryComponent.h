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
	Equipment		UMETA(DisplayName = "Equipment"),
	Attachment		UMETA(DisplayName = "Attachment"),
	Consumable		UMETA(DisplayName = "Consumable"),
	Throwable		UMETA(DisplayName = "Throwable"),
	Artifact		UMETA(DisplayName = "Artifact"),
	Misc			UMETA(DisplayName = "Misc")
};

UENUM(BlueprintType)
enum class EEquipType : uint8
{
	None			UMETA(DisplayName = "None"),
	Helmet			UMETA(DisplayName = "Helmet"),
	Vest			UMETA(DisplayName = "Vest"),
	Backpack		UMETA(DisplayName = "Backpack"),

	// [새로 추가할 전투 슬롯들]
	Weapon1			UMETA(DisplayName = "Primary Weapon"),
	Weapon2			UMETA(DisplayName = "Secondary Weapon"),
	Pistol			UMETA(DisplayName = "Pistol"),
	Throwable		UMETA(DisplayName = "Throwable")
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ItemData")
	float SpawnWeight = 10.0f;


	// [소모품용] 발동시킬 효과 클래스
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data")
	TSubclassOf<class UItemEffectBase> ItemEffectClass;

	// [무기용] 무기를 장착할 때 스폰할 실제 무기 액터 클래스
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data|Weapon")
	TSubclassOf<class AWeaponBase> WeaponClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data|Weapon")
	FRotator HolsterRotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data|Drop")
	TSubclassOf<class APickupItemBase> ItemClass;

	// [장비용] 캐릭터에게 입힐 3D 모델 (헬멧, 조끼, 가방 메쉬)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data|Equipment")
	TObjectPtr<class UStaticMesh> EquipmentMesh;

	// [장비 & 아티팩트용] 범용 스탯 보너스 수치
	// (예: 조끼면 방어력 50 증가, 가방이면 용량 50 증가, 아티팩트면 데미지 10% 증가 등)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data|Stats")
	float StatBonus = 0.0f;

	// ==========================================
	// [추가] 장비 전용 스탯
	// ==========================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data|Equipment")
	EEquipType EquipType = EEquipType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data|Equipment")
	int32 ItemLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data|Equipment")
	float MaxDurability = 100.0f;

	// 방어력 (예: 0.3 이면 30% 데미지 감소)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data|Equipment")
	float DefensePower = 0.0f;
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

	// [추가] 현재 이 슬롯에 있는 아이템의 남은 내구도
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CurrentDurability = 0.0f;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Data")
	class UDataTable* ItemDataTable;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void UseItemAtIndex(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void UseItemByID(FName UseItemID);

	// ==========================================
	// [추가] 현재 착용 중인 장비 정보
	// ==========================================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Helmet")
	FName EquippedHelmetID = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Helmet")
	float CurrentHelmetDurability = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Vest")
	FName EquippedVestID = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Vest")
	float CurrentVestDurability = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Backpack")
	FName EquippedBackpackID = NAME_None;

	// 장착된 아이템을 해제하는 함수
	void UnequipItemByID(FName ItemID);

	// [새로 추가] 전투 무기 장착 상태
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Combat")
	FName EquippedWeapon1ID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Combat")
	FName EquippedWeapon2ID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Combat")
	FName EquippedPistolID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Combat")
	FName EquippedThrowableID;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetTotalItemCount(FName ItemID) const;

protected:
	virtual void BeginPlay() override;

	// 가방의 총 칸 수 (배그의 가방 용량 개념)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 MaxSlots = 20;
};