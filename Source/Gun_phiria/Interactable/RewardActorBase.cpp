#include "RewardActorBase.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Kismet/GameplayStatics.h"

ARewardActorBase::ARewardActorBase()
{
	PrimaryActorTick.bCanEverTick = false;

	// 루트 컴포넌트: 메시 설정
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;
	MeshComp->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	// 상호작용 트리거 구역 설정
	InteractSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractSphere"));
	InteractSphere->SetupAttachment(RootComponent);
	InteractSphere->SetSphereRadius(150.0f);
	InteractSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	// 대기 파티클 설정
	IdleParticle = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("IdleParticle"));
	IdleParticle->SetupAttachment(RootComponent);

	InteractPromptText = TEXT("Get Reward");
	bIsClaimed = false;
}

void ARewardActorBase::BeginPlay()
{
	Super::BeginPlay();
}

void ARewardActorBase::Interact_Implementation(AActor* Interactor)
{
	// 이미 획득한 상태라면 무시
	if (bIsClaimed) return;

	// 1. 보상 지급 실행 (블루프린트에서 덮어쓴 로직이 여기서 실행됨)
	GiveReward(Interactor);

	// 2. 지급 완료 처리 (사운드, 이펙트, 파괴)
	OnRewardClaimed();
}

FString ARewardActorBase::GetInteractText_Implementation()
{
	// 획득 전이면 텍스트를 띄우고, 획득 후면 텍스트를 숨김
	return bIsClaimed ? TEXT("") : InteractPromptText;
}

void ARewardActorBase::GiveReward_Implementation(AActor* Interactor)
{
	// 기본 부모 클래스에서는 아무것도 지급하지 않습니다.
	// BP_GoldChest, BP_ExpOrb 등에서 이 함수를 덮어쓰기(Override) 해야 합니다.
}

void ARewardActorBase::OnRewardClaimed()
{
	bIsClaimed = true;

	// 획득 사운드 재생
	if (RewardSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, RewardSound, GetActorLocation());
	}

	// 획득 이펙트 재생
	if (RewardEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), RewardEffect, GetActorLocation(), GetActorRotation());
	}

	// 시각적 요소 숨기기 & 충돌 끄기 (자연스러운 파괴 연출을 위함)
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);

	// 2초 뒤 액터 완전히 메모리에서 파괴
	SetLifeSpan(2.0f);
}