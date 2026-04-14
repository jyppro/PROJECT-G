#include "InventoryStudio.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "../Weapon/WeaponBase.h"
#include "Animation/AnimationAsset.h"
#include "GameFramework/Character.h"

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

void AInventoryStudio::UpdateStudioEquipment(UStaticMesh* NewHelmet, UStaticMesh* NewVest, UStaticMesh* NewBackpack, AWeaponBase* EquippedWeapon, EStudioAnimType AnimType)
{
	if (CloneHelmetMesh) CloneHelmetMesh->SetStaticMesh(NewHelmet);
	if (CloneVestMesh) CloneVestMesh->SetStaticMesh(NewVest);
	if (CloneBackpackMesh) CloneBackpackMesh->SetStaticMesh(NewBackpack);

	//if (CloneWeaponMesh)
	//{
	//	if (EquippedWeapon && EquippedWeapon->GetWeaponMesh())
	//	{
	//		// 1. 메쉬 외형 복사
	//		CloneWeaponMesh->SetStaticMesh(EquippedWeapon->GetWeaponMesh()->GetStaticMesh());

	//		// 원본 무기의 "WeaponMesh"가 가진 트랜스폼을 통째로 가져오기
	//		FTransform OriginalMeshTransform = EquippedWeapon->GetWeaponMesh()->GetRelativeTransform();

	//		// 스튜디오 마네킹 손에 부착 (상대 좌표를 유지한 채로)
	//		FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepRelative, true);
	//		CloneWeaponMesh->AttachToComponent(CloneBodyMesh, AttachRules, FName("WeaponSocket"));

	//		// 가져온 트랜스폼(위젯에서 맞춘 0.1 스케일, -90도 회전 등)을 그대로 덮어씌움
	//		CloneWeaponMesh->SetRelativeTransform(OriginalMeshTransform);
	//	}
	//	else
	//	{
	//		CloneWeaponMesh->SetStaticMesh(nullptr);
	//	}
	//}

	if (CloneWeaponMesh)
	{
		if (EquippedWeapon && EquippedWeapon->GetWeaponMesh())
		{
			// 1. 메쉬 외형 복사
			CloneWeaponMesh->SetStaticMesh(EquippedWeapon->GetWeaponMesh()->GetStaticMesh());

			// =========================================================================
			// [가장 완벽한 해결책] 
			// 진짜 플레이어가 진짜 총을 어떻게 쥐고 있는지 계산해서 마네킹에게 복사합니다!
			// =========================================================================

			// 스튜디오를 생성할 때 Owner를 Player로 설정했으므로, 스튜디오의 주인을 가져옵니다.
			ACharacter* PlayerChar = Cast<ACharacter>(GetOwner());

			if (PlayerChar && PlayerChar->GetMesh())
			{
				// 진짜 총기 메쉬의 월드 좌표 (절대 위치)
				FTransform RealMeshWorld = EquippedWeapon->GetWeaponMesh()->GetComponentTransform();

				// 진짜 캐릭터 손바닥 소켓의 월드 좌표 (절대 위치)
				FTransform RealSocketWorld = PlayerChar->GetMesh()->GetSocketTransform(FName("WeaponSocket"));

				// 두 좌표를 빼서(GetRelativeTransform) 손바닥 기준으로 총이 정확히 어떻게 쥐어져 있는지 구합니다!
				// (여기에 스케일, 회전, 소켓 그립 오프셋이 전부 다 포함되어 있습니다)
				FTransform RelativeToHand = RealMeshWorld.GetRelativeTransform(RealSocketWorld);

				// 스튜디오 마네킹의 손바닥 소켓에 가짜 무기를 부착!
				FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepRelative, true);
				CloneWeaponMesh->AttachToComponent(CloneBodyMesh, AttachRules, FName("WeaponSocket"));

				// 방금 구한 "완벽한 그립 트랜스폼"을 가짜 메쉬에 덮어씌웁니다!
				CloneWeaponMesh->SetRelativeTransform(RelativeToHand);
			}
			else
			{
				// (혹시라도 플레이어를 못 찾았을 때를 대비한 안전 장치)
				FTransform OriginalMeshTransform = EquippedWeapon->GetWeaponMesh()->GetRelativeTransform();
				FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepRelative, true);
				CloneWeaponMesh->AttachToComponent(CloneBodyMesh, AttachRules, FName("WeaponSocket"));
				CloneWeaponMesh->SetRelativeTransform(OriginalMeshTransform);
			}
		}
		else
		{
			CloneWeaponMesh->SetStaticMesh(nullptr);
		}
	}

	// =========================================================================
	// [애니메이션 재생 부분 유지]
	// =========================================================================
	if (CloneBodyMesh)
	{
		UAnimationAsset* TargetAnim = DefaultIdleAnim; // 기본은 권총/맨손 대기 모션

		// 만약 소총 애니메이션 상태로 넘어왔다면 소총 모션으로 변경
		if (AnimType == EStudioAnimType::Rifle && RifleIdleAnim)
		{
			TargetAnim = RifleIdleAnim;
		}

		// 해당 애니메이션을 무한 반복(true)으로 재생합니다.
		if (TargetAnim)
		{
			CloneBodyMesh->PlayAnimation(TargetAnim, true);
		}
	}

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