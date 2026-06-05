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
		// 새로운 무기를 인벤토리에 지급 (기존 InventoryComponent 기능 활용)
		Player->PlayerInventory->AddItem(StoredRewardData.TargetWeaponItemID, 1);
		Player->PlayerInventory->UseItemByID(StoredRewardData.TargetWeaponItemID);
	}
	else if (StoredRewardData.RewardType == EAnvilRewardType::UpgradeWeapon)
	{
		// 현재 들고 있는 무기 스탯 강화
		if (AWeaponBase* CurrentWeapon = Player->GetCurrentWeapon())
		{
			CurrentWeapon->MinWeaponDamage *= StoredRewardData.DamageMultiplier;
			CurrentWeapon->MaxWeaponDamage *= StoredRewardData.DamageMultiplier;
			CurrentWeapon->FireRate *= StoredRewardData.FireRateMultiplier;

			// 강화 성공 효과음/이펙트 재생 가능
		}
	}

	// 2. 부모 스크린 닫기 (입력 모드 원상복구)
	if (ParentScreen.IsValid())
	{
		ParentScreen->CloseAnvilUI();
	}
}