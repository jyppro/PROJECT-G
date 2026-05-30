#include "ShopNPC.h"
#include "Engine/DataTable.h"
#include "Kismet/GameplayStatics.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BrainComponent.h"
#include "../Gun_phiriaCharacter.h"
#include "../Gun_phiriaGameInstance.h"
#include "../Weapon/WeaponBase.h"
#include "GameFramework/CharacterMovementComponent.h"

AShopNPC::AShopNPC()
{
	PrimaryActorTick.bCanEverTick = true;

	// 상점 주인의 이동 속도를 일반 적보다 훨씬 빠르게 설정
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = BossWalkSpeed;

		GetCharacterMovement()->MaxStepHeight = 100.0f;

		// 2. AI가 난간이나 단상 끝에서 바닥으로 떨어지는(뛰어내리는) 길찾기를 허용합니다.
		GetCharacterMovement()->bCanWalkOffLedges = true;
		GetCharacterMovement()->bCanWalkOffLedgesWhenCrouching = true;
	}

	DefaultWeaponClass = nullptr;
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// 떨어뜨리는 골드도 보스급으로 설정 (부모 클래스의 변수 덮어쓰기)
	MinGoldDrop = 300;
	MaxGoldDrop = 500;
}

void AShopNPC::BeginPlay()
{
	Super::BeginPlay(); // 부모의 BeginPlay 실행 (여기서 MaxHealth 등이 세팅됨)

	// 상점 주인은 적대화 전까지 완벽한 Idle 애니메이션 고정
	MaxHealth *= BossHealthMultiplier;
	CurrentHealth = MaxHealth;

	bIsAiming = false; // 절대로 먼저 조준 자세를 잡지 않도록 끔
	bIsHostile = false;

	GenerateRandomShopItems();

	// 뇌(AI) 정지 로직은 그대로 유지하여 움직이지 않게 만듦
	if (AAIController* AICon = Cast<AAIController>(GetController()))
	{
		if (BehaviorTreeAsset)
		{
			AICon->RunBehaviorTree(BehaviorTreeAsset);
			if (AICon->GetBrainComponent())
			{
				AICon->GetBrainComponent()->PauseLogic("PeacefulShopkeeper");
			}
		}
	}
}

float AShopNPC::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead) return 0.f;

	if (!bIsHostile && DamageAmount > 0.f)
	{
		bIsHostile = true;
		bIsAiming = true;

		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("You shouldn't have done that..."));

		if (HiddenWeaponClass)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			CurrentWeapon = GetWorld()->SpawnActor<AWeaponBase>(HiddenWeaponClass, GetActorLocation(), GetActorRotation(), SpawnParams);

			if (CurrentWeapon)
			{
				FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
				CurrentWeapon->AttachToComponent(GetMesh(), AttachmentRules, FName("WeaponSocket"));

				CurrentWeapon->MinWeaponDamage *= BossDamageMultiplier;
				CurrentWeapon->MaxWeaponDamage *= BossDamageMultiplier;
				CurrentWeapon->FireRate *= 5.0f;
			}
		}

		if (AAIController* AICon = Cast<AAIController>(GetController()))
		{
			if (AICon->GetBrainComponent())
			{
				AICon->GetBrainComponent()->ResumeLogic("PeacefulShopkeeper");
			}

			if (UBlackboardComponent* BlackboardComp = AICon->GetBlackboardComponent())
			{
				ACharacter* PlayerChar = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
				BlackboardComp->SetValueAsObject(FName("TargetActor"), PlayerChar);

				// 쿨다운 계산 및 블랙보드 전달
				if (CurrentWeapon && CurrentWeapon->FireRate > 0.0f)
				{
					// FireRate(분당 발사수)를 초당 발사 간격으로 변환
					float FireDelay = 60.0f / CurrentWeapon->FireRate;
					BlackboardComp->SetValueAsFloat(FName("AttackCooldown"), FireDelay);
				}
				else
				{
					// 무기가 없거나 계산 불가 시 기본 딜레이 (안전 장치)
					BlackboardComp->SetValueAsFloat(FName("AttackCooldown"), 0.5f);
				}

				BlackboardComp->SetValueAsBool(FName("UseBurst"), true);
			}
		}
	}

	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

void AShopNPC::Die(AController* Killer)
{
	Super::Die(Killer); // 골드 드랍 및 래그돌 처리

	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, TEXT("Shopkeeper Defeated!"));

	if (UGun_phiriaGameInstance* GameInst = Cast<UGun_phiriaGameInstance>(GetGameInstance()))
	{
		GameInst->bIsShopKeeperKilled = true;
	}
}

void AShopNPC::GenerateRandomShopItems()
{
	if (!ItemDataTable) return;

	TArray<FName> AllRowNames = ItemDataTable->GetRowNames();
	TArray<FName> ValidItemNames;

	for (const FName& RowName : AllRowNames)
	{
		if (RowName.ToString().StartsWith(TEXT("Item")))
		{
			ValidItemNames.Add(RowName);
		}
	}

	if (ValidItemNames.IsEmpty()) return;

	for (int32 i = ValidItemNames.Num() - 1; i > 0; i--)
	{
		int32 j = FMath::RandRange(0, i);
		ValidItemNames.Swap(i, j);
	}

	int32 ItemsToPick = FMath::Min(10, ValidItemNames.Num());

	for (int32 i = 0; i < ItemsToPick; i++)
	{
		FName ItemID = ValidItemNames[i];
		FItemData* ItemData = ItemDataTable->FindRow<FItemData>(ItemID, TEXT("ShopGen"));

		if (ItemData)
		{
			int32 RandomQty = (ItemData->MaxStackSize > 1) ? FMath::RandRange(5, 15) : 1;
			ShopInventory.Add(ItemID, RandomQty);
		}
	}
}