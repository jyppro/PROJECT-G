#include "InventoryStudio.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"

AInventoryStudio::AInventoryStudio()
{
	PrimaryActorTick.bCanEverTick = false; // 틱 연산 필요 없음 (최적화)

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// 1. 가짜 몸통 세팅
	CloneBodyMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CloneBodyMesh"));
	CloneBodyMesh->SetupAttachment(RootComponent);
	CloneBodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CloneBodyMesh->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));

	// 2. 가짜 장비들 세팅
	CloneHelmetMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CloneHelmetMesh"));
	CloneHelmetMesh->SetupAttachment(CloneBodyMesh, FName("HelmetSocket"));
	CloneHelmetMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	CloneVestMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CloneVestMesh"));
	CloneVestMesh->SetupAttachment(CloneBodyMesh, FName("VestSocket"));
	CloneVestMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	CloneBackpackMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CloneBackpackMesh"));
	CloneBackpackMesh->SetupAttachment(CloneBodyMesh, FName("BackpackSocket"));
	CloneBackpackMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	CloneWeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CloneWeaponMesh"));
	CloneWeaponMesh->SetupAttachment(CloneBodyMesh, FName("WeaponSocket"));
	CloneWeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 3. 캡처 카메라 세팅
	StudioCamera = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("StudioCamera"));
	StudioCamera->SetupAttachment(RootComponent);
	StudioCamera->SetRelativeLocation(FVector((-73.5f, 110.0f, 90.0f)));
	StudioCamera->SetRelativeRotation(FRotator(0.0f, 0.0f, -55.0f));

	// [최적화 핵심] 매 프레임 찍지 마! 움직일 때도 찍지 마!
	StudioCamera->bCaptureEveryFrame = false;
	StudioCamera->bCaptureOnMovement = false;
	StudioCamera->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;

	FPostProcessSettings& PPSettings = StudioCamera->PostProcessSettings;
	PPSettings.bOverride_AutoExposureBias = true; // 자동 노출 설정을 무시하고 직접 제어합니다.
	PPSettings.AutoExposureBias = 3.0f;
}

void AInventoryStudio::BeginPlay()
{
	Super::BeginPlay();

	// ShowOnlyList에 내 메쉬들만 등록
	if (StudioCamera)
	{
		StudioCamera->ShowOnlyComponent(CloneBodyMesh);
		StudioCamera->ShowOnlyComponent(CloneHelmetMesh);
		StudioCamera->ShowOnlyComponent(CloneVestMesh);
		StudioCamera->ShowOnlyComponent(CloneBackpackMesh);
		StudioCamera->ShowOnlyComponent(CloneWeaponMesh);
	}
}

// 플레이어가 본인 장비를 바꿀 때마다 이 함수를 호출해줄 겁니다.
void AInventoryStudio::UpdateStudioEquipment(UStaticMesh* NewHelmet, UStaticMesh* NewVest, UStaticMesh* NewBackpack, UStaticMesh* NewWeapon)
{
	if (CloneHelmetMesh) CloneHelmetMesh->SetStaticMesh(NewHelmet);
	if (CloneVestMesh) CloneVestMesh->SetStaticMesh(NewVest);
	if (CloneBackpackMesh) CloneBackpackMesh->SetStaticMesh(NewBackpack);
	if (CloneWeaponMesh) CloneWeaponMesh->SetStaticMesh(NewWeapon);

	// 옷을 갈아입었으니 사진을 한 방 찍어줍니다.
	CaptureProfile();
}

// 인벤토리가 열릴 때 "수동"으로 한 번만 캡처! (렉 제로)
void AInventoryStudio::CaptureProfile()
{
	if (StudioCamera)
	{
		StudioCamera->CaptureScene();
	}
}

void AInventoryStudio::SetCameraLive(bool bIsLive)
{
	if (StudioCamera)
	{
		// true면 매 프레임 캡처(애니메이션 재생), false면 캡처 중지
		StudioCamera->bCaptureEveryFrame = bIsLive;
	}
}