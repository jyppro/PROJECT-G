#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ThrowableProjectileBase.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

UCLASS()
class GUN_PHIRIA_API AThrowableProjectileBase : public AActor
{
	GENERATED_BODY()

public:
	AThrowableProjectileBase();

	// 투척 무기에서 전달해주는 속도와 남은 시간을 받아서 세팅하는 함수
	UFUNCTION(BlueprintCallable, Category = "Throwable")
	void InitializeThrow(FVector InitialVelocity, float FuseTime);

protected:
	virtual void BeginPlay() override;

	// 충돌 영역 (공 모양)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> CollisionComp;

	// 메쉬 (수류탄, 연막탄 모델링)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ProjectileMesh;

	// 물리 기반 포물선 비행을 담당하는 핵심 컴포넌트!
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	// 쿠킹 후 남은 폭발 시간 (손에서 2초 들고 던졌으면, 3초 뒤 폭발해야 함)
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Explosion")
	float TimeToExplode = 3.0f;

	// 자식 클래스(수류탄, 연막탄 등)에서 오버라이드하여 각자의 효과를 구현할 가상 함수!
	UFUNCTION(BlueprintCallable, Category = "Explosion")
	virtual void Explode();
};