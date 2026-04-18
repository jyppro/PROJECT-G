#include "ThrowableProjectileBase.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "TimerManager.h"

AThrowableProjectileBase::AThrowableProjectileBase()
{
	// 비행체 자체는 매 프레임 업데이트될 필요가 없으므로 꺼둡니다 (최적화)
	PrimaryActorTick.bCanEverTick = false;

	// 1. 충돌체 설정 (가장 바깥쪽 부모)
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	CollisionComp->InitSphereRadius(5.0f);
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComp->SetCollisionResponseToAllChannels(ECR_Block); // 모두 튕겨나감
	CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore); // 던진 본인은 통과
	CollisionComp->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	CollisionComp->SetSimulatePhysics(true);
	RootComponent = CollisionComp;

	// 2. 외형 메쉬 설정 (수류탄 모델링)
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(RootComponent);

	// 실제 충돌은 CollisionComp가 담당하므로 메쉬 자체의 충돌은 끕니다. (버그 방지)
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 3. 발사체 무브먼트 컴포넌트 (배그 투척물 물리의 핵심!)
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->InitialSpeed = 0.0f; // 처음엔 0 (InitializeThrow에서 덮어씌움)
	ProjectileMovement->MaxSpeed = 3000.0f;
	ProjectileMovement->bRotationFollowsVelocity = true; // 날아가는 방향을 바라보며 회전

	// [중요] 바닥이나 벽에 닿았을 때 튕기도록 설정!
	ProjectileMovement->bShouldBounce = true;
	ProjectileMovement->Bounciness = 0.3f; // 탱탱볼처럼 튀지 않게 탄성은 낮게 (0.3)
	ProjectileMovement->Friction = 0.6f;   // 마찰력을 높여서 바닥에 닿으면 금방 멈추게 함
}

void AThrowableProjectileBase::BeginPlay()
{
	Super::BeginPlay();
}

void AThrowableProjectileBase::InitializeThrow(FVector InitialVelocity, float FuseTime)
{
	// 1. 발사체의 물리적 날아가는 힘(속도) 설정
	if (ProjectileMovement)
	{
		ProjectileMovement->Velocity = InitialVelocity;
	}

	// 2. 전달받은 남은 시간(FuseTime) 뒤에 Explode() 함수 실행!
	TimeToExplode = FuseTime;

	FTimerHandle ExplodeTimerHandle;
	GetWorld()->GetTimerManager().SetTimer(ExplodeTimerHandle, this, &AThrowableProjectileBase::Explode, TimeToExplode, false);
}

void AThrowableProjectileBase::Explode()
{
	// 이 함수는 부모의 빈 껍데기입니다. 
	// 실제 폭발 데미지와 이펙트는 자식 클래스(AGrenadeProjectile)에서 override하여 구현합니다.

	Destroy(); // 기본적으로 자기 자신을 파괴
}