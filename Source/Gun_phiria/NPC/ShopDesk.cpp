#include "ShopDesk.h"
#include "ShopNPC.h"
#include "../Gun_phiriaCharacter.h"
#include "../UI/InventoryMainWidget.h"

// 새로 추가된 컴포넌트들을 사용하기 위한 헤더 파일들입니다.
#include "Components/SceneComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/BoxComponent.h"

AShopDesk::AShopDesk()
{
	PrimaryActorTick.bCanEverTick = false;

	// 1. 루트 씬 생성: 이 액터의 가장 기본이 되는 보이지 않는 기준점입니다.
	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	RootComponent = RootScene;

	// 2. 자식 액터 컴포넌트 생성 및 루트에 부착: 여기에 나무상자 블루프린트가 들어갑니다.
	DeskBlueprintVisual = CreateDefaultSubobject<UChildActorComponent>(TEXT("DeskBlueprintVisual"));
	DeskBlueprintVisual->SetupAttachment(RootComponent);

	// 3. 상호작용 감지용 박스 생성 및 루트에 부착
	InteractCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractCollision"));
	InteractCollision->SetupAttachment(RootComponent);

	// 박스의 크기를 적당히 넉넉하게 설정합니다. (에디터에서 수정 가능)
	InteractCollision->SetBoxExtent(FVector(100.f, 100.f, 80.f));

	// 레이캐스트(플레이어의 시선)에 박스가 닿을 수 있도록 설정합니다.
	InteractCollision->SetCollisionResponseToAllChannels(ECR_Ignore); // 우선 다 무시하고
	InteractCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); // 시야(Visibility) 채널만 블록합니다.
}

FString AShopDesk::GetInteractText_Implementation()
{
	if (!LinkedNPC) return TEXT("Closed (No NPC)");

	// 1. 살아있는데 화가 났을 때
	if (!LinkedNPC->IsDead() && LinkedNPC->GetIsHostile()) return TEXT("Shop is closed!");

	// 2. NPC가 죽었을 때
	if (LinkedNPC->IsDead()) return TEXT("Loot the Shop");

	// 3. 평상시
	return TEXT("Open Shop");
}

void AShopDesk::Interact_Implementation(AActor* Interactor)
{
	if (!LinkedNPC) return;

	// 1. 살아서 공격 중일 때: 상호작용 차단
	if (!LinkedNPC->IsDead() && LinkedNPC->GetIsHostile())
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("The shopkeeper is attacking you!"));
		return;
	}

	// 2. 평상시 OR 죽었을 때: 상점 UI 정상적으로 열기
	AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(Interactor);
	if (Player && Player->InventoryWidgetInstance)
	{
		if (!Player->bIsInventoryOpen) Player->ToggleInventory();

		UInventoryMainWidget* MainWidget = Cast<UInventoryMainWidget>(Player->InventoryWidgetInstance);
		if (MainWidget)
		{
			MainWidget->CurrentShopNPC = LinkedNPC;
			MainWidget->OpenShopMode(LinkedNPC->ShopInventory);

			// 이 부분에서 UI 위젯에게 "지금 NPC가 죽었으니 약탈 모드야!"라고 알려주어야 합니다.
			MainWidget->bIsShopLootMode = LinkedNPC->IsDead(); 
		}
	}
}