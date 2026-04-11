#include "PickupItemBase.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "../Gun_phiriaCharacter.h"
#include "Engine/DataTable.h"
#include "../component/InventoryComponent.h"
#include "../UI/InventoryMainWidget.h"
#include "Components/SceneComponent.h"

APickupItemBase::APickupItemBase()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* DefaultRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultRoot"));
	RootComponent = DefaultRoot;

	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	ItemMesh->SetupAttachment(RootComponent); // 루트 아래로 들어갑니다!

	ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ItemMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	ItemMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	OverlapSphere = CreateDefaultSubobject<USphereComponent>(TEXT("OverlapSphere"));
	OverlapSphere->SetupAttachment(RootComponent);
	OverlapSphere->SetSphereRadius(150.0f);

	OverlapSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	OverlapSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	OverlapSphere->SetCollisionResponseToChannel(ECC_Visibility, ECR_Overlap);

	Quantity = 1;
}

void APickupItemBase::Interact_Implementation(AActor* Interactor)
{
	if (AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(Interactor))
	{
		if (UInventoryComponent* Inventory = Player->PlayerInventory)
		{
			// 1. 가방에 넣기 시도! (반환값은 가방에 못 들어가고 남은 개수)
			int32 Leftover = Inventory->AddItem(ItemID, Quantity);

			// 2. 남은 게 없다면? (전부 다 주웠음!)
			if (Leftover <= 0)
			{
				Destroy(); // 바닥에서 아이템 파괴
			}
			// 3. 남은 게 있다면? (가방이 꽉 차서 다 못 주웠음!)
			else
			{
				// 바닥에 남은 개수를 업데이트
				Quantity = Leftover;

				// 화면에 "가방이 꽉 찼습니다!" 문구 띄우기
				// 방법 A: 간단한 디버그 메시지 (일단 이걸로 테스트 해봐!)
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("가방이 꽉 찼습니다!"));
				}

				// 방법 B: 진짜 상용 게임처럼 UI(HUD)에 띄우기 (네가 만든 HUD가 있다면 주석 풀고 사용!)
				/*
				if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
				{
					if (AGun_phiriaHUD* HUD = Cast<AGun_phiriaHUD>(PC->GetHUD()))
					{
						HUD->ShowSystemMessage(TEXT("가방이 꽉 찼습니다!")); // HUD에 만든 함수 이름으로 변경
					}
				}
				*/

				// 아이템 개수가 줄어들었으니, 인벤토리가 열려있다면 주변(Nearby) UI도 갱신해주기
				if (Player->bIsInventoryOpen && Player->InventoryWidgetInstance)
				{
					// 기존 UUserWidget을 우리가 만든 UInventoryMainWidget으로 형변환(Cast) 합니다.
					if (UInventoryMainWidget* MainWidget = Cast<UInventoryMainWidget>(Player->InventoryWidgetInstance))
					{
						// 복잡한 FindFunction 없이, 그냥 C++ 함수를 직접 똭! 호출합니다.
						MainWidget->ForceNearbyRefresh();
					}
				}
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