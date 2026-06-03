#pragma once

#include "CoreMinimal.h"
#include "RewardActorBase.h" // 이전에 만든 보상 베이스 클래스 상속 (경로에 맞게 수정)
#include "EnchantActor.generated.h"

UCLASS()
class GUN_PHIRIA_API AEnchantActor : public ARewardActorBase
{
	GENERATED_BODY()

public:
	AEnchantActor();

protected:
	virtual void BeginPlay() override;

	// --------------------------------------------------
	// 1. 시각적 컴포넌트 (삼각형 컨테이너 & 박스 3개)
	// --------------------------------------------------
	// 삼각형 베이스 메시는 부모 클래스(ARewardActorBase)의 MeshComp를 그대로 사용합니다.

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UStaticMeshComponent> EnchantBox1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UStaticMeshComponent> EnchantBox2;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UStaticMeshComponent> EnchantBox3;

	// --------------------------------------------------
	// 2. 인챈트 데이터
	// --------------------------------------------------
	// 이 액터가 스폰될 때 결정될 1~3 사이의 인챈트 횟수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enchant")
	int32 EnchantCount;

	// 박스 개수에 맞춰 시각적 상태를 업데이트하는 함수
	UFUNCTION(BlueprintCallable, Category = "Enchant")
	void UpdateBoxVisibility();

public:
	// 상호작용 시 호출될 함수 (부모 클래스 오버라이드)
	virtual void Interact_Implementation(AActor* Interactor) override;
};