#include "AnvilInteractable.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "../UI/AnvilScreenWidget.h"
#include "../Reward/RewardData.h"
#include "../Gun_phiriaCharacter.h"
#include "../Component/InventoryComponent.h"

AAnvilInteractable::AAnvilInteractable()
{
	PrimaryActorTick.bCanEverTick = false;

	AnvilMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AnvilMesh"));
	RootComponent = AnvilMesh;
}

void AAnvilInteractable::BeginPlay()
{
	Super::BeginPlay();
}

FString AAnvilInteractable::GetInteractText_Implementation()
{
	// 이미 사용했다면 텍스트를 다르게 띄워줍니다.
	return bIsUsed ? TEXT("이미 사용한 모루입니다.") : TEXT("모루 사용하기 (F)");
}

void AAnvilInteractable::Interact_Implementation(AActor* Interactor)
{
	if (bIsUsed || !AnvilRewardTable || !AnvilUIClass) return;

	// 1. 플레이어 인벤토리 검사 (현재 들고 있는 무기 파악)
	AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(Interactor);
	if (!Player || !Player->PlayerInventory) return;

	TArray<FName> EquippedWeapons;
	if (!Player->PlayerInventory->EquippedWeapon1ID.IsNone()) EquippedWeapons.Add(Player->PlayerInventory->EquippedWeapon1ID);
	if (!Player->PlayerInventory->EquippedWeapon2ID.IsNone()) EquippedWeapons.Add(Player->PlayerInventory->EquippedWeapon2ID);

	int32 WeaponCount = EquippedWeapons.Num();

	// 2. 조건에 맞는 보상만 임시 풀(Pool)에 담기
	TArray<FAnvilRewardData*> ValidRewards;
	TArray<FName> AllRowNames = AnvilRewardTable->GetRowNames();

	for (FName RowName : AllRowNames)
	{
		FAnvilRewardData* Data = AnvilRewardTable->FindRow<FAnvilRewardData>(RowName, TEXT(""));
		if (!Data) continue;

		if (Data->RewardType == EAnvilRewardType::GiveNewWeapon)
		{
			// 무기가 2개 미만일 때만 신규 무기 선택지를 제공
			if (WeaponCount < 2) ValidRewards.Add(Data);
		}
		else if (Data->RewardType == EAnvilRewardType::WeaponUpgrade)
		{
			// 플레이어가 '요구 무기'를 장착하고 있을 때만 해당 진화 선택지 제공
			if (EquippedWeapons.Contains(Data->RequiredWeaponID)) ValidRewards.Add(Data);
		}
	}

	// 3. 필터링된 배열에서 무작위 3개 셔플 및 추출 (기존 로직 응용)
	if (ValidRewards.Num() > 0)
	{
		for (int32 i = ValidRewards.Num() - 1; i > 0; i--)
		{
			int32 RandomIndex = FMath::RandRange(0, i);
			ValidRewards.Swap(i, RandomIndex);
		}
	}

	TArray<FAnvilRewardData*> SelectedRewards;
	int32 MaxChoices = FMath::Min(3, ValidRewards.Num());
	for (int32 i = 0; i < MaxChoices; i++)
	{
		SelectedRewards.Add(ValidRewards[i]);
	}

	// 4. 모루 UI 화면에 띄우기 (이전과 동일)
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC)
	{
		UAnvilScreenWidget* AnvilWidget = CreateWidget<UAnvilScreenWidget>(PC, AnvilUIClass);
		if (AnvilWidget)
		{
			AnvilWidget->SetupAnvilUI(SelectedRewards);
			AnvilWidget->AddToViewport();

			UGameplayStatics::SetGamePaused(GetWorld(), true);
			FInputModeUIOnly InputMode;
			PC->SetInputMode(InputMode);
			PC->SetShowMouseCursor(true);

			bIsUsed = true;
		}
	}
}