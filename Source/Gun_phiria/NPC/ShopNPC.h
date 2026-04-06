#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h" // AActor 대신 ACharacter 포함
#include "../Interface/InteractInterface.h"
#include "ShopNPC.generated.h"

UCLASS()
class GUN_PHIRIA_API AShopNPC : public ACharacter, public IInteractInterface
{
	GENERATED_BODY()

public:
	// 생성자
	AShopNPC();

	// 상호작용 인터페이스 구현
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual FString GetInteractText_Implementation() override;

	// 나중에 적대화(전투) 시스템을 위해 데미지 처리 함수를 미리 선언해 둡니다.
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	// 2. NPC가 현재 보유한 상점 재고 목록 (ItemID, 보유 수량)
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Shop Info")
	TMap<FName, int32> ShopInventory;

protected:
	// 이 NPC가 현재 플레이어에게 화가 났는지(적대적인지) 여부
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shop")
	bool bIsHostile = false;

	// NPC의 체력 (약탈 시스템용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop|Stats")
	float MaxHealth = 500.0f; // 상점 주인은 보통 엄청 강하게 설정하죠!

	float CurrentHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop Info")
	TArray<FName> ItemsForSale;

	// 1. 아이템 데이터 테이블 지정용 (에디터에서 넣어주세요)
	UPROPERTY(EditAnywhere, Category = "Shop Info")
	class UDataTable* ItemDataTable;

	virtual void BeginPlay() override;

	// 랜덤 상점 생성 함수
	void GenerateRandomShopItems();
};