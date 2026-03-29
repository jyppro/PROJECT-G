#include "InventoryMainWidget.h"
#include "Components/ScrollBox.h"
#include "TimerManager.h"
#include "../Gun_phiriaCharacter.h"
#include "../Item/PickupItemBase.h"

void UInventoryMainWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 인벤토리가 켜지면 0.2초마다 CheckNearbyItems 함수를 반복 실행하는 타이머 작동!
	GetWorld()->GetTimerManager().SetTimer(NearbyCheckTimer, this, &UInventoryMainWidget::CheckNearbyItems, 0.2f, true);
}

void UInventoryMainWidget::NativeDestruct()
{
	Super::NativeDestruct();

	// 인벤토리가 꺼지면 쓸데없이 돌아가지 않게 타이머 종료
	GetWorld()->GetTimerManager().ClearTimer(NearbyCheckTimer);
}

void UInventoryMainWidget::ForceNearbyRefresh()
{
	// 개수를 -1로 강제 초기화해서, 다음 타이머 틱(0.2초)때 무조건 다시 그리게 만듭니다.
	LastNearbyCount = -1;
	CheckNearbyItems(); // 즉시 1회 업데이트 실행
}

void UInventoryMainWidget::CheckNearbyItems()
{
	if (AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwningPlayerPawn()))
	{
		TArray<APickupItemBase*> NearbyItems = Player->GetNearbyItems();

		if (NearbyItems.Num() != LastNearbyCount)
		{
			LastNearbyCount = NearbyItems.Num();

			// C++에서 직접 위젯을 그리지 않고, 블루프린트의 이벤트를 호출해서 배열만 넘겨줌!
			UpdateNearbyUI(NearbyItems);
		}
	}
}