#include "EnchantActor.h"
#include "Components/StaticMeshComponent.h"
#include "Math/UnrealMathUtility.h"

AEnchantActor::AEnchantActor()
{
	// 1. 박스 컴포넌트 생성 및 부모 메시(삼각형 컨테이너)에 부착
	EnchantBox1 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EnchantBox1"));
	EnchantBox1->SetupAttachment(MeshComp); // 부모 클래스의 MeshComp(삼각형)에 부착

	EnchantBox2 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EnchantBox2"));
	EnchantBox2->SetupAttachment(MeshComp);

	EnchantBox3 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EnchantBox3"));
	EnchantBox3->SetupAttachment(MeshComp);

	// 기본 충돌 설정 (박스는 단순히 시각적 요소이므로 충돌을 꺼줍니다)
	EnchantBox1->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EnchantBox2->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EnchantBox3->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	EnchantCount = 1; // 기본값
}

void AEnchantActor::BeginPlay()
{
	Super::BeginPlay();

	// 2. 스폰 시 1 ~ 3 사이의 랜덤한 인챈트 횟수 부여
	// (만약 DungeonGenerator 등에서 확정적으로 횟수를 정해주고 싶다면 여기서 랜덤을 빼면 돼!)
	EnchantCount = FMath::RandRange(1, 3);

	// 3. 횟수에 맞춰 박스 시각화 업데이트
	UpdateBoxVisibility();
}

void AEnchantActor::UpdateBoxVisibility()
{
	// EnchantCount 수치에 따라 박스를 숨깁니다 (최소 1개는 항상 보이게)
	EnchantBox1->SetVisibility(EnchantCount >= 1);
	EnchantBox2->SetVisibility(EnchantCount >= 2);
	EnchantBox3->SetVisibility(EnchantCount >= 3);
}

void AEnchantActor::Interact_Implementation(AActor* Interactor)
{
	// 이미 횟수를 다 썼다면 무시
	if (EnchantCount <= 0 || bIsClaimed) return;

	// TODO: 블루프린트나 C++로 플레이어의 아티팩트 레벨업 UI를 띄우는 로직 실행

	// 상호작용 성공 시 횟수 차감 및 박스 끄기
	EnchantCount--;
	UpdateBoxVisibility();

	if (EnchantCount <= 0)
	{
		// 횟수를 모두 소진하면 부모 클래스의 OnRewardClaimed()를 호출하여 액터 비활성화/파괴
		OnRewardClaimed();
	}
}