#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ShopNPC.generated.h"

UCLASS()
class GUN_PHIRIA_API AShopNPC : public ACharacter
{
	GENERATED_BODY()

public:
	AShopNPC();

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	// NPC가 죽었을 때 아이템을 드롭할 함수입니다.
	void DropLoot();

	// 상점 테이블에서 NPC의 재고를 참조해야 하므로 public 유지
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Shop Info")
	TMap<FName, int32> ShopInventory;

	// 플레이어가 떨어진 아이템을 스폰할 때 사용할 아이템 클래스
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop Info")
	TSubclassOf<class APickupItemBase> PickupItemClass;

	// NPC가 죽었는지 여부를 확인하는 함수
	UFUNCTION(BlueprintCallable, Category = "Shop")
	bool IsDead() const { return CurrentHealth <= 0.0f; }

	bool GetIsHostile() const { return bIsHostile; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shop")
	bool bIsHostile = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop|Stats")
	float MaxHealth = 500.0f;

	float CurrentHealth;

	UPROPERTY(EditAnywhere, Category = "Shop Info")
	class UDataTable* ItemDataTable;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override; // AI 추적을 위해 추가

	void GenerateRandomShopItems();

private:
	// 공격받았을 때 플레이어를 추적하기 위한 포인터
	class AGun_phiriaCharacter* TargetPlayer = nullptr;

	// AI 추적용 타이머
	FTimerHandle AttackTimerHandle;
	void PerformMeleeAttack();
};