#include "InventoryComponent.h"
#include "Engine/DataTable.h"
#include "../Item/ItemEffectBase.h"
#include "../Gun_phiriaCharacter.h"
#include "Blueprint/UserWidget.h" // ADD THIS LINE
#include "../UI/InventoryMainWidget.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// 게임이 시작될 때, 지정된 칸 수(MaxSlots)만큼 빈 슬롯을 미리 만들어 둡니다.
	InventorySlots.SetNum(MaxSlots);
}

int32 UInventoryComponent::AddItem(FName ItemID, int32 Quantity)
{
	if (Quantity <= 0 || ItemID.IsNone() || !ItemDataTable) return Quantity;

	FItemData* ItemInfo = ItemDataTable->FindRow<FItemData>(ItemID, TEXT("AddItem_WeightCheck"));
	if (!ItemInfo) return Quantity;

	int32 AddableQuantity = Quantity;
	if (ItemInfo->ItemWeight > 0.0f)
	{
		float AvailableWeight = MaxWeight - CurrentWeight;
		if (AvailableWeight < ItemInfo->ItemWeight) return Quantity;

		int32 MaxAffordable = FMath::FloorToInt(AvailableWeight / ItemInfo->ItemWeight);
		AddableQuantity = FMath::Min(Quantity, MaxAffordable);
	}

	if (AddableQuantity <= 0) return Quantity;

	// =========================================================
	// 1. 가방에 똑같은 아이템이 이미 있는지 먼저 확인 (겹치기)
	// =========================================================
	for (int32 i = 0; i < InventorySlots.Num(); i++)
	{
		// 빈 슬롯이 아니고, 아이템 ID가 일치한다면?
		if (!InventorySlots[i].IsEmpty() && InventorySlots[i].ItemID == ItemID)
		{
			InventorySlots[i].Quantity += AddableQuantity;
			CurrentWeight += (ItemInfo->ItemWeight * AddableQuantity);

			return Quantity - AddableQuantity;
		}
	}

	// =========================================================
	// 2. 똑같은 아이템이 없다면 빈 슬롯을 찾아서 새로 넣기
	// =========================================================
	for (int32 i = 0; i < InventorySlots.Num(); i++)
	{
		if (InventorySlots[i].IsEmpty())
		{
			InventorySlots[i].ItemID = ItemID;
			InventorySlots[i].Quantity = AddableQuantity;
			CurrentWeight += (ItemInfo->ItemWeight * AddableQuantity);

			return Quantity - AddableQuantity;
		}
	}

	return Quantity;
}

bool UInventoryComponent::RemoveItem(FName ItemID, int32 Quantity)
{
	if (Quantity <= 0 || ItemID.IsNone() || !ItemDataTable) return false;

	// 데이터 테이블에서 정보 찾기 (버릴 때 무게를 빼주기 위함)
	FItemData* ItemInfo = ItemDataTable->FindRow<FItemData>(ItemID, TEXT("RemoveItem"));

	for (int32 i = 0; i < InventorySlots.Num(); i++)
	{
		if (InventorySlots[i].ItemID == ItemID)
		{
			// 지우려는 개수와 실제 슬롯에 있는 개수 중 작은 값을 뺌
			int32 RemovedAmount = FMath::Min(Quantity, InventorySlots[i].Quantity);
			InventorySlots[i].Quantity -= RemovedAmount;

			// 가방 무게 줄여주기!
			if (ItemInfo)
			{
				CurrentWeight -= (ItemInfo->ItemWeight * RemovedAmount);
				// 소수점 오차 방지
				if (CurrentWeight < 0.0f) CurrentWeight = 0.0f;
			}

			if (InventorySlots[i].Quantity <= 0)
			{
				InventorySlots[i].ItemID = NAME_None;
				InventorySlots[i].Quantity = 0;
			}
			return true;
		}
	}
	return false;
}

void UInventoryComponent::UseItemAtIndex(int32 SlotIndex)
{
	// 유효한 슬롯인지, 아이템이 들어있는지 확인
	if (!InventorySlots.IsValidIndex(SlotIndex) || InventorySlots[SlotIndex].IsEmpty()) return;

	FName ItemID = InventorySlots[SlotIndex].ItemID;
	FItemData* ItemInfo = ItemDataTable->FindRow<FItemData>(ItemID, TEXT("UseItem"));

	if (ItemInfo && ItemInfo->ItemEffectClass)
	{
		// 캐릭터 가져오기
		AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwner());

		// C++의 마법: 클래스의 '기본 객체(CDO)'를 가져와서 즉시 함수 실행! (메모리 생성/삭제 비용 0)
		UItemEffectBase* EffectCDO = ItemInfo->ItemEffectClass->GetDefaultObject<UItemEffectBase>();

		if (EffectCDO && EffectCDO->UseItem(Player))
		{
			// 사용에 성공(true)했다면 아이템 1개 소모
			RemoveItem(ItemID, 1);
		}
	}
}

void UInventoryComponent::UseItemByID(FName UseItemID)
{
	if (UseItemID.IsNone() || !ItemDataTable) return;

	// 1. 가방 안에 진짜 이 아이템이 있는지 먼저 검사!
	bool bHasItem = false;
	for (int32 i = 0; i < InventorySlots.Num(); i++)
	{
		if (InventorySlots[i].ItemID == UseItemID && InventorySlots[i].Quantity > 0)
		{
			bHasItem = true;
			break;
		}
	}

	if (!bHasItem) return; // 가방에 없으면 실행 안 함

	// 2. 데이터 테이블에서 아이템 정보 가져오기
	FItemData* ItemInfo = ItemDataTable->FindRow<FItemData>(UseItemID, TEXT("UseItem"));
	if (!ItemInfo) return;

	AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(GetOwner());
	if (!Player) return;

	// [추가됨] 아이템 사용/장착이 '성공'했는지 추적하는 변수
	bool bUseSuccess = false;

	// 3. 아이템 타입에 따른 분기 처리 (핵심!)
	switch (ItemInfo->ItemType)
	{
	case EItemType::Consumable:
	case EItemType::Throwable:
	case EItemType::Artifact:
	{
		// [소모품/투척/아티팩트] : 기존처럼 ItemEffectClass를 이용해 효과 발동
		if (ItemInfo->ItemEffectClass)
		{
			UItemEffectBase* EffectCDO = ItemInfo->ItemEffectClass->GetDefaultObject<UItemEffectBase>();

			if (EffectCDO && EffectCDO->UseItem(Player))
			{
				bUseSuccess = true; // 효과 발동 성공!
			}
		}
		break;
	}

	case EItemType::Equipment:
	{
		// [장비] : 뚝배기, 조끼, 가방 등
		// TODO: 캐릭터 클래스에 EquipItem 같은 함수를 만들어서 여기서 호출할 예정입니다.
		// 예: Player->EquipItem(ItemInfo);

		// 지금은 테스트용으로 메시지만 띄웁니다.
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan, TEXT("장비 장착 시도!"));
		// bUseSuccess = true; // 나중에 장착 구현이 완료되면 주석을 푸세요.
		break;
	}

	case EItemType::Weapon:
	{
		// [무기] : 총기 장착
		// TODO: 무기 스폰 및 손에 쥐여주는 로직 호출 예정

		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("무기 장착 시도!"));
		// bUseSuccess = true;
		break;
	}

	default:
		break;
	}

	// 4. 사용/장착에 성공했다면 아이템을 소모하고 UI를 갱신합니다.
	if (bUseSuccess)
	{
		// 인벤토리에서 1개 삭제
		RemoveItem(UseItemID, 1);

		// UI 새로고침 (블루프린트 방식에서 C++ 방식으로 깔끔하게 변경!)
		if (Player->bIsInventoryOpen && Player->InventoryWidgetInstance)
		{
			// 우리가 만든 C++ 위젯 클래스로 형변환하여 직접 함수 호출
			if (UInventoryMainWidget* MainWidget = Cast<UInventoryMainWidget>(Player->InventoryWidgetInstance))
			{
				MainWidget->RefreshInventory();
			}
		}
	}
}