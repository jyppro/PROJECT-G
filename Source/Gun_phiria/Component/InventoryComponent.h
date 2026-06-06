#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "InventoryComponent.generated.h"

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
	Pistol			UMETA(DisplayName = "Pistol"),
	Throwable		UMETA(DisplayName = "Throwable")
};

USTRUCT(BlueprintType)
struct FItemData : public FTableRowBase
{
	GENERATED_BODY()

public:
	// --- 공통 기본 정보 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Base")
	FName ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Base")
	FText ItemName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Base")
	FText ItemDescription;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Base")
	TObjectPtr<class UTexture2D> ItemIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Base")
	EItemType ItemType = EItemType::Misc;

	// 무기가 아닐 때만 스폰 가중치, 무게, 스택, 드랍 클래스 표시 (무기는 모루에서만 획득하므로 숨김)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Base", meta = (EditCondition = "ItemType != EItemType::Weapon"))
	float ItemWeight = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Base", meta = (EditCondition = "ItemType != EItemType::Weapon"))
	int32 MaxStackSize = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Base", meta = (EditCondition = "ItemType != EItemType::Weapon"))
	float SpawnWeight = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Base", meta = (EditCondition = "ItemType != EItemType::Weapon"))
	TSubclassOf<class APickupItemBase> ItemClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Base", meta = (EditCondition = "ItemType != EItemType::Weapon"))
	int32 DefaultSpawnQuantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Base", meta = (EditCondition = "ItemType != EItemType::Weapon"))
	int32 BuyPrice = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Base", meta = (EditCondition = "ItemType != EItemType::Weapon"))
	int32 SellPrice = 0;

	// --- 소비/아티팩트 전용 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Consumable", meta = (EditCondition = "ItemType == EItemType::Consumable || ItemType == EItemType::Artifact"))
	TSubclassOf<class UItemEffectBase> ItemEffectClass;

	// --- 무기 전용 (ItemType이 Weapon일 때만 활성화) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Weapon", meta = (EditCondition = "ItemType == EItemType::Weapon"))
	TSubclassOf<class AWeaponBase> WeaponClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Weapon", meta = (EditCondition = "ItemType == EItemType::Weapon"))
	FRotator HolsterRotationOffset = FRotator::ZeroRotator;

	// --- 장비(방어구/가방) 전용 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Equipment", meta = (EditCondition = "ItemType == EItemType::Equipment"))
	EEquipType EquipType = EEquipType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Equipment", meta = (EditCondition = "ItemType == EItemType::Equipment"))
	TObjectPtr<class UStaticMesh> EquipmentMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Equipment", meta = (EditCondition = "ItemType == EItemType::Equipment"))
	float StatBonus = 0.0f; // 가방의 무게 증가량 등

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Equipment", meta = (EditCondition = "ItemType == EItemType::Equipment"))
	float MaxDurability = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Equipment", meta = (EditCondition = "ItemType == EItemType::Equipment"))
	float DefensePower = 0.0f;
};

USTRUCT(BlueprintType)
struct FInventorySlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Quantity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CurrentDurability = 0.0f;

	bool IsEmpty() const { return Quantity <= 0 || ItemID.IsNone(); }
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GUN_PHIRIA_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 AddItem(FName ItemID, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(FName ItemID, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void UseItemAtIndex(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void UseItemByID(FName UseItemID);

	void UnequipItemByID(FName ItemID);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetTotalItemCount(FName ItemID) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Inventory")
	TArray<FInventorySlot> InventorySlots;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory|Weight")
	float CurrentWeight = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Weight")
	float MaxWeight = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Data")
	TObjectPtr<UDataTable> ItemDataTable; // Raw 포인터를 TObjectPtr로 변경

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Combat")
	FName EquippedWeapon1ID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Combat")
	FName EquippedWeapon2ID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Combat")
	FName EquippedPistolID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Combat")
	FName EquippedThrowableID;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 MaxSlots = 20;
};