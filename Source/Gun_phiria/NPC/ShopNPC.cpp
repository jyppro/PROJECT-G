#include "ShopNPC.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AShopNPC::AShopNPC()
{
	PrimaryActorTick.bCanEverTick = true;

	// ACharacter는 기본적으로 CapsuleComponent와 Mesh를 제공하므로, 설정만 만져줍니다.
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// 플레이어의 스캔(상호작용)과 총알에 맞도록 콜리전 설정
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	CurrentHealth = MaxHealth;
}

FString AShopNPC::GetInteractText_Implementation()
{
	// 적대 상태면 상호작용 텍스트를 띄우지 않습니다.
	if (bIsHostile) return TEXT("");

	return TEXT("Open Shop");
}

void AShopNPC::Interact_Implementation(AActor* Interactor)
{
	// 화가 난 상태면 상점을 열어주지 않습니다.
	if (bIsHostile) return;

	// TODO: 상점 UI 오픈 로직
	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, TEXT("NPC Shop UI Opened"));
}

float AShopNPC::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	// 플레이어에게 맞으면 적대 상태로 돌입합니다!
	if (!bIsHostile && ActualDamage > 0.f)
	{
		bIsHostile = true;
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("Shop owner is hostile! Combat started!"));

		// TODO: 여기서 AI Controller에게 "플레이어를 공격해!"라고 명령을 내리게 될 겁니다.
	}

	CurrentHealth -= ActualDamage;
	if (CurrentHealth <= 0.0f)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, TEXT("Shop owner defeated! Loot available!"));
		// TODO: 사망 처리 로직 및 바닥에 아이템 스폰 (약탈 시스템)
		Destroy(); // 임시로 파괴
	}

	return ActualDamage;
}