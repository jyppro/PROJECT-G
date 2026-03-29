#include "PickupItemBase.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "../Gun_phiriaCharacter.h"
#include "Engine/DataTable.h"
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
	FString DisplayName = ItemID.ToString(); // 기본값은 ID (실패 대비)

	// 1. 플레이어 캐릭터를 통해 인벤토리 컴포넌트에 접근
	if (AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetWorld()->GetFirstPlayerController()->GetPawn()))
	{
		if (UInventoryComponent* Inventory = Player->PlayerInventory)
		{
			// 2. 인벤토리 컴포넌트가 가지고 있는 데이터 테이블(ItemDataTable) 확인
			if (UDataTable* ItemTable = Inventory->ItemDataTable)
			{
				// 3. 내 ItemID로 데이터 테이블에서 행(Row)을 찾기
				FItemData* ItemInfo = ItemTable->FindRow<FItemData>(ItemID, TEXT("PickupItem_GetText"));

				if (ItemInfo)
				{
					// 4. 찾았다면, 진짜 예쁜 이름(ItemName)으로 바꿔치기!
					DisplayName = ItemInfo->ItemName.ToString();
				}
			}
		}
	}

	// 5. 텍스트 완성해서 반환 (예: "[F] First Aid Kit")
	// 만약 개수가 1개 초과라면 개수도 표시 (예: "[F] 5.56mm (30)")
	if (Quantity > 1)
	{
		return FString::Printf(TEXT("%s (%d)"), *DisplayName, Quantity);
	}
	else
	{
		return FString::Printf(TEXT("%s"), *DisplayName);
	}
}