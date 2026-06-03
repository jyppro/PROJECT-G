#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Interface/InteractInterface.h" // 경로가 다를 경우 프로젝트에 맞게 수정해 주세요.
#include "RewardActorBase.generated.h"

UCLASS()
class GUN_PHIRIA_API ARewardActorBase : public AActor, public IInteractInterface
{
	GENERATED_BODY()

public:
	ARewardActorBase();

protected:
	virtual void BeginPlay() override;

	// --------------------------------------------------
	// 1. 시각적 및 충돌 컴포넌트
	// --------------------------------------------------
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UStaticMeshComponent> MeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class USphereComponent> InteractSphere; // 상호작용 인식 범위

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UParticleSystemComponent> IdleParticle; // 대기 중 반짝임 이펙트

	// --------------------------------------------------
	// 2. 보상 획득 시 연출 세팅
	// --------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward|Effects")
	TObjectPtr<class USoundBase> RewardSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward|Effects")
	TObjectPtr<class UParticleSystem> RewardEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward")
	FString InteractPromptText;

	UPROPERTY(BlueprintReadOnly, Category = "Reward")
	bool bIsClaimed;

public:
	// --------------------------------------------------
	// 3. IInteractInterface 인터페이스 구현
	// --------------------------------------------------
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual FString GetInteractText_Implementation() override;

protected:
	// --------------------------------------------------
	// 4. 자식 블루프린트에서 오버라이드할 보상 지급 로직
	// --------------------------------------------------
	UFUNCTION(BlueprintNativeEvent, Category = "Reward")
	void GiveReward(AActor* Interactor);
	virtual void GiveReward_Implementation(AActor* Interactor);

	// 보상 획득 후 공통 처리 (파괴 및 이펙트)
	UFUNCTION(BlueprintCallable, Category = "Reward")
	virtual void OnRewardClaimed();
};