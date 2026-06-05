#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "RewardData.generated.h"

// ==============================================================================
// 1. [Base] 모든 선택지가 공유하는 공통 데이터 (UI에 그림을 그리기 위한 베이스)
// ==============================================================================
USTRUCT(BlueprintType)
struct FRewardDataBase : public FTableRowBase
{
	GENERATED_BODY()

public:
	// 선택지 이름 (예: "매직 완드", "화염의 부적")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward UI")
	FText ChoiceName;

	// 선택지 설명 (예: "공격이 '매직 미사일'로 변경됩니다.")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward UI")
	FText ChoiceDescription;

	// 선택지 아이콘 (UI에 띄울 이미지)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward UI")
	TObjectPtr<class UTexture2D> ChoiceIcon;
};

// ==============================================================================
// 2. [Derived] 모루(Anvil) 전용 보상 데이터
// ==============================================================================

// 모루 보상 타입 열거형
UENUM(BlueprintType)
enum class EAnvilRewardType : uint8
{
	GiveNewWeapon		UMETA(DisplayName = "Give New Weapon"),
	UpgradeWeapon		UMETA(DisplayName = "Upgrade Current Weapon")
};

// FRewardDataBase를 상속받음 (Name, Description, Icon을 기본으로 가짐)
USTRUCT(BlueprintType)
struct FAnvilRewardData : public FRewardDataBase
{
	GENERATED_BODY()

public:
	// 모루 로직 1: 어떤 방식의 강화인가?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anvil Logic")
	EAnvilRewardType RewardType = EAnvilRewardType::UpgradeWeapon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anvil Logic", meta = (EditCondition = "RewardType==EAnvilRewardType::WeaponStatBoost"))
	FName RequiredWeaponID;

	// 모루 로직 2: 새로운 무기를 줄 경우의 ItemID (기존 InventoryComponent 연동용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anvil Logic|New Weapon", meta = (EditCondition = "RewardType==EAnvilRewardType::GiveNewWeapon"))
	FName TargetWeaponItemID;

	// 모루 로직 3: 스탯 강화일 경우의 배율 (예: 1.15 = 15% 증가)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anvil Logic|Stat Boost", meta = (EditCondition = "RewardType==EAnvilRewardType::WeaponStatBoost"))
	float FireRateMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anvil Logic|Stat Boost", meta = (EditCondition = "RewardType==EAnvilRewardType::WeaponStatBoost"))
	float DamageMultiplier = 1.0f;
};

// ==============================================================================
// 3. [Derived] 아티팩트(Artifact) 전용 보상 데이터
// ==============================================================================

// FRewardDataBase를 상속받음
USTRUCT(BlueprintType)
struct FArtifactRewardData : public FRewardDataBase
{
	GENERATED_BODY()

public:
	// 아티팩트 로직: 기존 인벤토리 시스템에 만든 UItemEffectBase 또는 아티팩트 ItemID 연동
	// 네가 기획한 '관통', '크리티컬', '속성' 등을 제어할 아이템 ID를 넘겨줌
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Artifact Logic")
	FName ArtifactItemID;

	// (필요하다면 추가) 아티팩트 등급이나 획득 시 발동될 이펙트 클래스
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Artifact Logic")
	// TSubclassOf<class UItemEffectBase> ArtifactEffectClass;
};