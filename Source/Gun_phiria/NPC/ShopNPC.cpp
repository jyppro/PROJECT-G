#include "ShopNPC.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "../Gun_phiriaCharacter.h"
#include "../UI/InventoryMainWidget.h"
#include "../component/InventoryComponent.h"
#include "Engine/DataTable.h"


AShopNPC::AShopNPC()
{
	PrimaryActorTick.bCanEverTick = true;

	// ACharacter는 기본적으로 CapsuleComponent와 Mesh를 제공하므로, 설정만 만져줍니다.
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// 플레이어의 스캔(상호작용)과 총알에 맞도록 콜리전 설정
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	CurrentHealth = MaxHealth;
}

void AShopNPC::BeginPlay()
{
	Super::BeginPlay();

	// 게임 시작 시 랜덤 재고 생성
	GenerateRandomShopItems();
}

void AShopNPC::GenerateRandomShopItems()
{
	if (!ItemDataTable) return;

	// 데이터 테이블의 모든 행(Row) 이름을 가져옵니다.
	TArray<FName> RowNames = ItemDataTable->GetRowNames();
	if (RowNames.Num() == 0) return;

	// 배열을 랜덤하게 섞습니다. (Fisher-Yates Shuffle)
	for (int32 i = RowNames.Num() - 1; i > 0; i--)
	{
		int32 j = FMath::RandRange(0, i);
		RowNames.Swap(i, j);
	}

	// 최대 10개의 아이템을 뽑습니다.
	int32 ItemsToPick = FMath::Min(10, RowNames.Num());

	for (int32 i = 0; i < ItemsToPick; i++)
	{
		FName ItemID = RowNames[i];
		FItemData* ItemData = ItemDataTable->FindRow<FItemData>(ItemID, TEXT("ShopGen"));

		if (ItemData)
		{
			// MaxStackSize가 1보다 크면(소비/재료 아이템) 5~15개 랜덤, 장비면 무조건 1개
			int32 RandomQty = (ItemData->MaxStackSize > 1) ? FMath::RandRange(5, 15) : 1;
			ShopInventory.Add(ItemID, RandomQty);
		}
	}
}

FString AShopNPC::GetInteractText_Implementation()
{
	// 적대 상태면 상호작용 텍스트를 띄우지 않습니다.
	if (bIsHostile) return TEXT("");

	return TEXT("Open Shop");
}

void AShopNPC::Interact_Implementation(AActor* Interactor)
{
	if (bIsHostile) return;

	AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(Interactor);
	if (Player && Player->InventoryWidgetInstance)
	{
		if (!Player->bIsInventoryOpen) Player->ToggleInventory();

		UInventoryMainWidget* MainWidget = Cast<UInventoryMainWidget>(Player->InventoryWidgetInstance);
		if (MainWidget)
		{
			// 현재 거래 중인 NPC를 위젯에 저장하고, 재고를 넘겨줍니다.
			MainWidget->CurrentShopNPC = this;
			MainWidget->OpenShopMode(ShopInventory);
		}
	}
}

float AShopNPC::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	// 플레이어에게 맞으면 적대 상태로 돌입합니다!
	if (!bIsHostile && ActualDamage > 0.f)
	{
		bIsHostile = true;
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("Shop owner is hostile! Combat started!"));

		// TODO: 여기서 AI Controller에게 "플레이어를 공격해!"라고 명령을 내리게 될 겁니다.
	}

	CurrentHealth -= ActualDamage;
	if (CurrentHealth <= 0.0f)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, TEXT("Shop owner defeated! Loot available!"));
		// TODO: 사망 처리 로직 및 바닥에 아이템 스폰 (약탈 시스템)
		Destroy(); // 임시로 파괴
	}

	return ActualDamage;
}