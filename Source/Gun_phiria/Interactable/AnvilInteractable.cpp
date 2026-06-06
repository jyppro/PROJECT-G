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
	return bIsUsed ? TEXT("Already used") : TEXT("Use Anvil (F)");
}

void AAnvilInteractable::Interact_Implementation(AActor* Interactor)
{
	if (bIsUsed || !AnvilRewardTable || !AnvilUIClass) return;

	AGun_phiriaCharacter* Player = Cast<AGun_phiriaCharacter>(Interactor);
	if (!Player || !Player->PlayerInventory) return;

	// 1. 플레이어 보유 무기 검사
	TArray<FName> EquippedWeapons;
	if (!Player->PlayerInventory->EquippedWeapon1ID.IsNone()) EquippedWeapons.Add(Player->PlayerInventory->EquippedWeapon1ID);
	if (!Player->PlayerInventory->EquippedWeapon2ID.IsNone()) EquippedWeapons.Add(Player->PlayerInventory->EquippedWeapon2ID);

	int32 WeaponCount = EquippedWeapons.Num();

	// 2. 무기 선택지와 업그레이드 선택지를 각각 담을 배열 준비
	TArray<FAnvilRewardData*> WeaponPool;
	TArray<FAnvilRewardData*> UpgradePool;

	TArray<FName> AllRowNames = AnvilRewardTable->GetRowNames();
	for (FName RowName : AllRowNames)
	{
		FAnvilRewardData* Data = AnvilRewardTable->FindRow<FAnvilRewardData>(RowName, TEXT(""));
		if (!Data) continue;

		if (Data->RewardType == EAnvilRewardType::GiveNewWeapon && WeaponCount < 2)
		{
			WeaponPool.Add(Data);
		}
		else if (Data->RewardType == EAnvilRewardType::WeaponUpgrade && EquippedWeapons.Contains(Data->RequiredWeaponID))
		{
			UpgradePool.Add(Data);
		}
	}

	// 3. 섞고 3개 뽑는 공통 람다 함수
	auto ShuffleAndPick = [](TArray<FAnvilRewardData*>& Pool) -> TArray<FAnvilRewardData*>
		{
			if (Pool.Num() > 0)
			{
				for (int32 i = Pool.Num() - 1; i > 0; i--)
				{
					int32 RandomIndex = FMath::RandRange(0, i);
					Pool.Swap(i, RandomIndex);
				}
			}
			TArray<FAnvilRewardData*> Result;
			int32 MaxChoices = FMath::Min(3, Pool.Num());
			for (int32 i = 0; i < MaxChoices; i++) Result.Add(Pool[i]);
			return Result;
		};

	// 4. 최종적으로 UI에 넘겨줄 3개짜리 배열 생성
	TArray<FAnvilRewardData*> FinalWeaponRewards = ShuffleAndPick(WeaponPool);
	TArray<FAnvilRewardData*> FinalUpgradeRewards = ShuffleAndPick(UpgradePool);

	// 5. 처음에 띄워줄 탭 결정 (무기가 2개 꽉 찼으면 무조건 업그레이드 탭부터)
	EAnvilRewardType StartMode = (WeaponCount == 2) ? EAnvilRewardType::WeaponUpgrade : EAnvilRewardType::GiveNewWeapon;

	// 6. UI 띄우기 및 3개의 인자 전달!
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		UAnvilScreenWidget* AnvilWidget = CreateWidget<UAnvilScreenWidget>(PC, AnvilUIClass);
		if (AnvilWidget)
		{
			// [핵심 수정] 이제 에러가 나지 않도록 3개의 인자를 모두 넣어줍니다.
			AnvilWidget->SetupAnvilUI(FinalWeaponRewards, FinalUpgradeRewards, StartMode);
			AnvilWidget->AddToViewport();

			UGameplayStatics::SetGamePaused(GetWorld(), true);
			FInputModeUIOnly InputMode;
			PC->SetInputMode(InputMode);
			PC->SetShowMouseCursor(true);

			bIsUsed = true;
		}
	}
}