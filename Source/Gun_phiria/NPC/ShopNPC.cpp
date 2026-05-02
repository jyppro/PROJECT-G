#include "ShopNPC.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "AIController.h" // AI 이동을 위해 필요
#include "../Gun_phiriaCharacter.h"
#include "../Item/PickupItemBase.h" // 드롭할 아이템 클래스 (경로 확인 요망)
#include "Engine/DataTable.h"
#include "../Component/InventoryComponent.h"

AShopNPC::AShopNPC()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	CurrentHealth = MaxHealth;

	// 상점 주인은 매우 빠름!
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = 800.0f;
	}

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AShopNPC::BeginPlay()
{
	Super::BeginPlay();
	GenerateRandomShopItems();
}

// Tick 함수에서 플레이어를 향해 이동하도록 간단한 AI를 구현합니다.
void AShopNPC::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 적대 상태이고 타겟이 살아있으며, 나도 살아있다면 쫓아갑니다.
	if (bIsHostile && TargetPlayer && !TargetPlayer->GetIsDead() && CurrentHealth > 0.f)
	{
		AAIController* AICon = Cast<AAIController>(GetController());
		if (AICon)
		{
			// 플레이어 위치로 이동 명령 (NavMeshBoundsVolume이 맵에 깔려 있어야 작동합니다)
			AICon->MoveToLocation(TargetPlayer->GetActorLocation(), 50.f);
		}

		// 거리가 충분히 가까우면 공격 실행
		float DistanceToPlayer = FVector::Dist(GetActorLocation(), TargetPlayer->GetActorLocation());
		if (DistanceToPlayer < 150.0f) // 근접 공격 사거리
		{
			// 연속 공격 방지를 위해 타이머가 안 돌고 있을 때만 공격
			if (!GetWorldTimerManager().IsTimerActive(AttackTimerHandle))
			{
				PerformMeleeAttack();
			}
		}
	}
}

void AShopNPC::PerformMeleeAttack()
{
	if (!TargetPlayer) return;

	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange, TEXT("NPC SMASH!"));

	// 플레이어에게 데미지 전달 (매우 아픔!)
	UGameplayStatics::ApplyDamage(TargetPlayer, 100.0f, GetController(), this, UDamageType::StaticClass());

	// 1.5초마다 공격하도록 타이머 설정
	GetWorldTimerManager().SetTimer(AttackTimerHandle, 1.5f, false);
}

// 상호작용 관련 함수(Interact_Implementation 등)는 삭제합니다.

float AShopNPC::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (CurrentHealth <= 0.f) return 0.f; // 이미 죽었다면 데미지 무시

	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	// 처음 맞았을 때 적대 상태로 전환
	if (!bIsHostile && ActualDamage > 0.f)
	{
		bIsHostile = true;
		TargetPlayer = Cast<AGun_phiriaCharacter>(DamageCauser); // 때린 놈을 타겟으로 설정!
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("You shouldn't have done that."));
	}

	CurrentHealth -= ActualDamage;

	if (CurrentHealth <= 0.0f)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, TEXT("Shop owner defeated!"));
		DropLoot(); // 사망 시 아이템 드롭

		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 충돌 끄기
		GetMesh()->SetSimulatePhysics(true); // 시체처럼 쓰러지기 (래그돌)
		SetActorTickEnabled(false); // Tick 멈춤
	}

	return ActualDamage;
}

void AShopNPC::DropLoot()
{
	if (!PickupItemClass) return; // 드롭할 아이템 클래스가 에디터에 할당 안되어있으면 무시

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// 내 상점 인벤토리를 돌면서 바닥에 아이템을 쏟아냅니다.
	for (const auto& Elem : ShopInventory)
	{
		FName ItemID = Elem.Key;
		int32 Qty = Elem.Value;

		// 랜덤한 위치로 약간 흩뿌리며 스폰
		FVector RandomOffset = FMath::VRand() * FMath::RandRange(50.f, 150.f);
		RandomOffset.Z = 10.0f; // 바닥 위로 약간 띄움
		FVector SpawnLocation = GetActorLocation() + RandomOffset;

		APickupItemBase* DroppedItem = GetWorld()->SpawnActor<APickupItemBase>(PickupItemClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
		if (DroppedItem)
		{
			// 드롭된 아이템에 ID와 수량 정보 입력
			DroppedItem->ItemID = ItemID;
			DroppedItem->Quantity = Qty;
		}
	}

	// 드롭 완료 후 상점 재고 비우기 (중복 방지)
	ShopInventory.Empty();
}

void AShopNPC::GenerateRandomShopItems()
{
	if (!ItemDataTable) return;

	TArray<FName> AllRowNames = ItemDataTable->GetRowNames();
	TArray<FName> ValidItemNames;

	// 1. "Item"으로 시작하는 데이터만 먼저 골라냅니다!
	for (const FName& RowName : AllRowNames)
	{
		if (RowName.ToString().StartsWith(TEXT("Item")))
		{
			ValidItemNames.Add(RowName);
		}
	}

	// 조건에 맞는 아이템이 하나도 없다면 종료
	if (ValidItemNames.IsEmpty()) return;

	// 2. 유효한 아이템 배열을 무작위로 섞습니다. (Fisher-Yates Shuffle)
	for (int32 i = ValidItemNames.Num() - 1; i > 0; i--)
	{
		int32 j = FMath::RandRange(0, i);
		ValidItemNames.Swap(i, j);
	}

	// 3. 섞인 배열에서 최대 10개를 뽑습니다.
	int32 ItemsToPick = FMath::Min(10, ValidItemNames.Num());

	for (int32 i = 0; i < ItemsToPick; i++)
	{
		FName ItemID = ValidItemNames[i];
		FItemData* ItemData = ItemDataTable->FindRow<FItemData>(ItemID, TEXT("ShopGen"));

		if (ItemData)
		{
			// 소모품은 여러 개, 장비는 1개 설정
			int32 RandomQty = (ItemData->MaxStackSize > 1) ? FMath::RandRange(5, 15) : 1;
			ShopInventory.Add(ItemID, RandomQty);
		}
	}
}