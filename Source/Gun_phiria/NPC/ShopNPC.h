#pragma once

#include "CoreMinimal.h"
#include "../Enemy/EnemyCharacter.h"
#include "ShopNPC.generated.h"

UCLASS()
class GUN_PHIRIA_API AShopNPC : public AEnemyCharacter
{
	GENERATED_BODY()

public:
	AShopNPC();

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	// [추가] 부모의 죽음 로직을 덮어써서 상점만의 특별한 죽음 처리를 합니다.
	virtual void Die(AController* Killer) override;

	// 상점 고유 기능 유지
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Shop Info")
	TMap<FName, int32> ShopInventory;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop Info")
	TSubclassOf<class APickupItemBase> PickupItemClass;

	UFUNCTION(BlueprintCallable, Category = "Shop")
	bool IsDead() const { return bIsDead; } // 부모의 bIsDead 변수를 사용합니다.

	bool GetIsHostile() const { return bIsHostile; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shop")
	bool bIsHostile = false;

	// [핵심] 적군처럼 시작하자마자 무기를 들지 않도록, 숨겨둘 무기 클래스를 따로 파놓습니다.
	UPROPERTY(EditAnywhere, Category = "Combat")
	TSubclassOf<class AWeaponBase> HiddenWeaponClass;

	UPROPERTY(EditAnywhere, Category = "Shop Info")
	class UDataTable* ItemDataTable;

	void GenerateRandomShopItems();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	class UBehaviorTree* BehaviorTreeAsset;
};