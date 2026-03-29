#include "PickupItemBase.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "../Gun_phiriaCharacter.h"
#include "../component/InventoryComponent.h"

APickupItemBase::APickupItemBase()
{
	PrimaryActorTick.bCanEverTick = false;

	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	RootComponent = ItemMesh;
	ItemMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); // 라인트레이스에 맞도록 설정

	OverlapSphere = CreateDefaultSubobject<USphereComponent>(TEXT("OverlapSphere"));
	OverlapSphere->SetupAttachment(RootComponent);
	OverlapSphere->SetSphereRadius(150.0f);

	// 기본값
	Quantity = 1;
}

void APickupItemBase::Interact_Implementation(AActor* Interactor)
{
	if (AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(Interactor))
	{
		if (UInventoryComponent* Inventory = Player->PlayerInventory)
		{
			// 가방에 넣기 시도! (남은 개수를 반환받음)
			int32 Leftover = Inventory->AddItem(ItemID, Quantity);

			if (Leftover <= 0)
			{
				// 전부 다 주웠다면 월드에서 이 아이템을 파괴!
				Destroy();
			}
			else
			{
				// 가방이 꽉 차서 남았다면, 월드에 남은 개수만 업데이트
				Quantity = Leftover;
			}
		}
	}
}

FString APickupItemBase::GetInteractText_Implementation()
{
	// 예: "구급 상자 줍기 (2)"
	return FString::Printf(TEXT("%s"), *ItemID.ToString());
}