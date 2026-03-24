#include "TestInteractableActor.h"
#include "Components/StaticMeshComponent.h"

ATestInteractableActor::ATestInteractableActor()
{
	// 1. 스태틱 메시 컴포넌트 생성 및 루트로 설정
	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	RootComponent = StaticMesh;

	// 2. 캐릭터의 스캔 레이저(ECC_Visibility)에 블록되도록 설정
	StaticMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}

// F키를 눌렀을 때 실행되는 함수
void ATestInteractableActor::Interact_Implementation(AActor* Interactor)
{
	// 배틀그라운드처럼 아이템을 줍는 느낌을 내기 위해 자신을 파괴합니다.
	// (나중엔 여기서 Interactor(플레이어)의 인벤토리에 자신을 추가하는 로직을 넣으면 돼!)
	Destroy();

	// (디버그 확인용)
	// GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, TEXT("Test Actor Interaction & Destroyed!"));
}