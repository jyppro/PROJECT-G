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
	// 충돌이나 기타 기본 세팅은 부모(AEnemyCharacter)가 알아서 해줍니다!

	// 상점 주인의 압도적인 스피드 설정
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = 1000.0f;
	}

	// [핵심] 부모 클래스(AEnemyCharacter)가 시작하자마자 무기를 쥐여주는 것을 방지합니다.
	DefaultWeaponClass = nullptr;
}

void AShopNPC::BeginPlay()
{
	Super::BeginPlay(); // 부모의 BeginPlay 실행 (체력 초기화 등)
	GenerateRandomShopItems();

	// [수정] 게임 시작 직후 0.1초 대기 후 뇌를 정지시킵니다.
	// (AI 컨트롤러가 완전히 생성되기도 전에 끄는 것을 방지)
	FTimerHandle PauseTimerHandle;
	GetWorldTimerManager().SetTimer(PauseTimerHandle, FTimerDelegate::CreateLambda([this]()
		{
			if (AAIController* AICon = Cast<AAIController>(GetController()))
			{
				if (AICon->GetBrainComponent())
				{
					AICon->GetBrainComponent()->PauseLogic("PeacefulShopkeeper");
				}
			}
		}), 0.1f, false);
}

float AShopNPC::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead) return 0.f;

	// 1. 처음 맞았을 때 적대 상태로 전환
	if (!bIsHostile && DamageAmount > 0.f)
	{
		bIsHostile = true;
		bIsAiming = true; // 부모 클래스의 변수 (애니메이션용)

		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("You shouldn't have done that..."));

		// [핵심 수정 1] 무기를 정중앙(0,0,0)이 아닌 NPC의 현재 위치에 무조건(AlwaysSpawn) 스폰합니다!
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
			}
		}

		// [핵심 수정 2] 일시정지 해두었던 뇌를 깨우고, 총알이 아닌 '플레이어'를 확실하게 타겟으로 지정합니다!
		if (AAIController* AICon = Cast<AAIController>(GetController()))
		{
			// 트리가 멈춰있거나 꼬였을 수 있으니 아예 확실하게 처음부터 다시 켜줍니다.
			if (BehaviorTreeAsset)
			{
				AICon->RunBehaviorTree(BehaviorTreeAsset);
			}

			if (AICon->GetBrainComponent())
			{
				AICon->GetBrainComponent()->ResumeLogic("PeacefulShopkeeper");
			}

			if (UBlackboardComponent* BlackboardComp = AICon->GetBlackboardComponent())
			{
				// DamageCauser(총알) 대신 확실하게 PlayerCharacter를 가져와 타겟으로 설정합니다.
				ACharacter* PlayerChar = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
				BlackboardComp->SetValueAsObject(FName("TargetActor"), PlayerChar);
			}
		}
	}

	// 2. 체력 차감 로직은 부모(AEnemyCharacter)에게 온전히 맡깁니다.
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	return ActualDamage;
}

void AShopNPC::Die(AController* Killer)
{
	// 1. 부모의 죽음 처리 (무기 파괴, 래그돌 물리엔진, 골드 드랍 등)를 그대로 실행합니다.
	Super::Die(Killer);

	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, TEXT("Shop owner defeated!"));

	// 2. 상점 주인만의 특별한 죽음 처리 (다음 게임부터 상점 출현 금지)
	if (UGun_phiriaGameInstance* GameInst = Cast<UGun_phiriaGameInstance>(GetGameInstance()))
	{
		GameInst->bIsShopKeeperKilled = true;
	}
}

void AShopNPC::GenerateRandomShopItems()
{
	// (이전 코드와 완전히 동일하므로 그대로 두시면 됩니다!)
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