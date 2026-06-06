#include "AnvilChoiceSlotWidget.h"
#include "AnvilScreenWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "../Gun_phiriaCharacter.h"
#include "../Component/InventoryComponent.h"
#include "../Weapon/WeaponBase.h"

void UAnvilChoiceSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ChoiceButton)
	{
		ChoiceButton->OnClicked.AddDynamic(this, &UAnvilChoiceSlotWidget::OnButtonClicked);
	}
}

void UAnvilChoiceSlotWidget::SetupSlot(const FAnvilRewardData& InRewardData, UAnvilScreenWidget* InParentScreen)
{
	StoredRewardData = InRewardData;
	ParentScreen = InParentScreen;

	if (NameText) NameText->SetText(StoredRewardData.ChoiceName);
	if (DescriptionText) DescriptionText->SetText(StoredRewardData.ChoiceDescription);
	if (IconImage && StoredRewardData.ChoiceIcon) IconImage->SetBrushFromTexture(StoredRewardData.ChoiceIcon);
}

void UAnvilChoiceSlotWidget::OnButtonClicked()
{
	AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (!Player || !Player->PlayerInventory) return;

	// 1. 보상 적용 로직
	if (StoredRewardData.RewardType == EAnvilRewardType::GiveNewWeapon)
	{
		// 신규 무기 지급 (홀스터에 장착됨)
		Player->PlayerInventory->AddItem(StoredRewardData.TargetWeaponItemID, 1);
		Player->PlayerInventory->UseItemByID(StoredRewardData.TargetWeaponItemID);
	}
	else if (StoredRewardData.RewardType == EAnvilRewardType::WeaponUpgrade)
	{
		// 무기 진화 (교체): 기존 무기 장착 해제 및 인벤토리에서 삭제 후 새 무기 지급
		Player->PlayerInventory->UnequipItemByID(StoredRewardData.RequiredWeaponID);
		Player->PlayerInventory->RemoveItem(StoredRewardData.RequiredWeaponID, 1);

		Player->PlayerInventory->AddItem(StoredRewardData.TargetWeaponItemID, 1);
		Player->PlayerInventory->UseItemByID(StoredRewardData.TargetWeaponItemID);

		// TODO: 진화 성공 사운드나 팡 터지는 UI 이펙트 추가
	}

	// ====================================================================
	// [완벽 수정] 새 무기를 강제로 손에 쥐여주고, 탄창을 가득 채웁니다.
	// ====================================================================
	int32 TargetSlotIndex = 0;

	// 새로 얻은 무기가 인벤토리의 1번 슬롯인지 2번 슬롯인지 판별합니다.
	if (Player->PlayerInventory->EquippedWeapon1ID == StoredRewardData.TargetWeaponItemID)
	{
		TargetSlotIndex = 1;
	}
	else if (Player->PlayerInventory->EquippedWeapon2ID == StoredRewardData.TargetWeaponItemID)
	{
		TargetSlotIndex = 2;
	}

	// 무기가 정상적으로 슬롯에 들어갔다면
	if (TargetSlotIndex > 0)
	{
		// 1. 플레이어에게 홀스터에 있는 새 무기를 꺼내서 손에 들라고 명령합니다.
		Player->EquipWeaponSlot(TargetSlotIndex);

		// 2. 해당 슬롯의 실제 무기 액터에 접근하여 탄창을 100% 채워줍니다.
		if (Player->WeaponSlots.IsValidIndex(TargetSlotIndex) && Player->WeaponSlots[TargetSlotIndex])
		{
			AWeaponBase* NewWeapon = Player->WeaponSlots[TargetSlotIndex];
			NewWeapon->CurrentAmmo = NewWeapon->MagazineCapacity;
		}
	}

	// 3. 부모 스크린 닫기
	if (ParentScreen.IsValid())
	{
		ParentScreen->CloseAnvilUI();
	}
}