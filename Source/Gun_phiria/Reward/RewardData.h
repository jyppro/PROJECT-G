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
	GiveNewWeapon		UMETA(DisplayName = "Acquire new weapons"),
	WeaponUpgrade		UMETA(DisplayName = "Upgrade existing weapons")
};

// FRewardDataBase를 상속받음
USTRUCT(BlueprintType)
struct FAnvilRewardData : public FRewardDataBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anvil Logic")
	EAnvilRewardType RewardType = EAnvilRewardType::GiveNewWeapon;

	// [결과] 플레이어에게 실제로 지급할 무기의 Item ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anvil Logic")
	FName TargetWeaponItemID;

	// [조건] 업그레이드 노드일 경우, 플레이어가 가지고 있어야만 이 선택지가 뜨는 '기존 무기'의 Item ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anvil Logic|Upgrade", meta = (EditCondition = "RewardType==EAnvilRewardType::WeaponUpgrade"))
	FName RequiredWeaponID;
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