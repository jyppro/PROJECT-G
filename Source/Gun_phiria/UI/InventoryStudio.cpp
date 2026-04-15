#include "InventoryStudio.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "../Weapon/WeaponBase.h"
#include "Animation/AnimationAsset.h"
#include "GameFramework/Character.h"
#include "../Gun_phiriaCharacter.h"

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

	CloneWeaponMesh_Hand = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CloneWeaponMesh"));
	CloneWeaponMesh_Hand->SetupAttachment(CloneBodyMesh, FName("WeaponSocket"));
	CloneWeaponMesh_Hand->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	CloneWeaponMesh0 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CloneWeaponMesh0"));
	CloneWeaponMesh0->SetupAttachment(CloneBodyMesh);
	CloneWeaponMesh0->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	CloneWeaponMesh1 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CloneWeaponMesh1"));
	CloneWeaponMesh1->SetupAttachment(CloneBodyMesh);
	CloneWeaponMesh1->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	CloneWeaponMesh2 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CloneWeaponMesh2"));
	CloneWeaponMesh2->SetupAttachment(CloneBodyMesh);
	CloneWeaponMesh2->SetCollisionEnabled(ECollisionEnabled::NoCollision);

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

		StudioCamera->ShowOnlyComponent(CloneWeaponMesh_Hand);
		StudioCamera->ShowOnlyComponent(CloneWeaponMesh0);
		StudioCamera->ShowOnlyComponent(CloneWeaponMesh1);
		StudioCamera->ShowOnlyComponent(CloneWeaponMesh2);
	}
}

void AInventoryStudio::UpdateStudioEquipment(UStaticMesh* NewHelmet, UStaticMesh* NewVest, UStaticMesh* NewBackpack, AWeaponBase* EquippedWeapon, EStudioAnimType AnimType)
{
	// 1. 방어구 업데이트
	if (CloneHelmetMesh) CloneHelmetMesh->SetStaticMesh(NewHelmet);
	if (CloneVestMesh) CloneVestMesh->SetStaticMesh(NewVest);
	if (CloneBackpackMesh) CloneBackpackMesh->SetStaticMesh(NewBackpack);

	AGun_phiriaCharacter* PlayerChar = Cast<AGun_phiriaCharacter>(GetOwner());
	if (!PlayerChar) return;

	// =========================================================================
	// 2. [손에 들고 있는 무기] 처리 (CloneWeaponMesh_Hand)
	// =========================================================================
	if (CloneWeaponMesh_Hand)
	{
		if (EquippedWeapon && EquippedWeapon->GetWeaponMesh())
		{
			CloneWeaponMesh_Hand->SetStaticMesh(EquippedWeapon->GetWeaponMesh()->GetStaticMesh());

			if (PlayerChar->GetMesh())
			{
				FTransform RealMeshWorld = EquippedWeapon->GetWeaponMesh()->GetComponentTransform();
				FTransform RealSocketWorld = PlayerChar->GetMesh()->GetSocketTransform(FName("WeaponSocket"));
				FTransform RelativeToHand = RealMeshWorld.GetRelativeTransform(RealSocketWorld);

				FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepRelative, true);
				CloneWeaponMesh_Hand->AttachToComponent(CloneBodyMesh, AttachRules, FName("WeaponSocket"));
				CloneWeaponMesh_Hand->SetRelativeTransform(RelativeToHand);
			}
		}
		else
		{
			CloneWeaponMesh_Hand->SetStaticMesh(nullptr);
		}
	}

	// =========================================================================
	// 3. [등/홀스터에 보관된 무기] 처리 (크기 버그 완벽 해결!)
	// =========================================================================
	UStaticMeshComponent* HolsterCloneMeshes[3] = { CloneWeaponMesh0, CloneWeaponMesh1, CloneWeaponMesh2 };
	FName HolsterSockets[3] = { FName("PistolHolsterSocket"), FName("BackpackWeapon1Socket"), FName("BackpackWeapon2Socket") };

	for (int32 i = 0; i < 3; i++)
	{
		UStaticMeshComponent* TargetClone = HolsterCloneMeshes[i];
		if (!TargetClone) continue;

		if (PlayerChar->WeaponSlots.IsValidIndex(i) &&
			PlayerChar->WeaponSlots[i] &&
			PlayerChar->ActiveWeaponSlot != i &&
			PlayerChar->WeaponSlots[i]->GetWeaponMesh())
		{
			AWeaponBase* HolsteredWeapon = PlayerChar->WeaponSlots[i];

			TargetClone->SetStaticMesh(HolsteredWeapon->GetWeaponMesh()->GetStaticMesh());

			// [핵심 해결책] 손에 쥔 무기처럼 절대 좌표를 빼서 완벽한 트랜스폼을 계산합니다!
			FTransform RealMeshWorld = HolsteredWeapon->GetWeaponMesh()->GetComponentTransform();
			FTransform RealSocketWorld = PlayerChar->GetMesh()->GetSocketTransform(HolsterSockets[i]);
			FTransform RelativeToHolster = RealMeshWorld.GetRelativeTransform(RealSocketWorld);

			FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepRelative, true);
			TargetClone->AttachToComponent(CloneBodyMesh, AttachRules, HolsterSockets[i]);

			// 억지로 1.0 스케일이나 회전값을 넣지 않고, 실제 무기의 크기와 각도를 그대로 덮어씌움!
			TargetClone->SetRelativeTransform(RelativeToHolster);
		}
		else
		{
			TargetClone->SetStaticMesh(nullptr);
		}
	}

	// =========================================================================
	// 4. 애니메이션 및 캡처
	// =========================================================================
	if (CloneBodyMesh)
	{
		UAnimationAsset* TargetAnim = DefaultIdleAnim;
		if (AnimType == EStudioAnimType::Rifle && RifleIdleAnim) TargetAnim = RifleIdleAnim;
		if (TargetAnim) CloneBodyMesh->PlayAnimation(TargetAnim, true);
	}

	CaptureProfile();
}

// 인벤토리가 열릴 때 "수동"으로 한 번만 캡처! (렉 제로)
void AInventoryStudio::CaptureProfile()
{
	if (StudioCamera)
	{
		if (!StudioCamera->bCaptureEveryFrame)
		{
			StudioCamera->CaptureScene();
		}
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