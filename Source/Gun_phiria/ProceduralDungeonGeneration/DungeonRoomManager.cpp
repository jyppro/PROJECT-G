#include "DungeonRoomManager.h"
#include "../Enemy/EnemyCharacter.h"
#include "../UI/Gun_phiriaHUD.h"

// Engine Headers
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"

ADungeonRoomManager::ADungeonRoomManager()
{
	PrimaryActorTick.bCanEverTick = false;

	RoomTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("RoomTrigger"));
	RootComponent = RoomTrigger;
	RoomTrigger->SetCollisionProfileName(TEXT("Trigger"));
}

void ADungeonRoomManager::BeginPlay()
{
	Super::BeginPlay();

	if (RoomTrigger)
	{
		RoomTrigger->OnComponentBeginOverlap.AddDynamic(this, &ADungeonRoomManager::OnPlayerEnter);
	}
}

void ADungeonRoomManager::OnPlayerEnter(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bIsTriggered || bIsCleared) return;

	TObjectPtr<ACharacter> PlayerChar = Cast<ACharacter>(OtherActor);
	if (PlayerChar && PlayerChar->IsPlayerControlled())
	{
		bIsTriggered = true;
		LockDoors();
		SpawnEnemies();
	}
}

void ADungeonRoomManager::LockDoors()
{
	for (TObjectPtr<AActor> Door : ConnectedDoors)
	{
		if (Door)
		{
			Door->SetActorHiddenInGame(false);
			Door->SetActorEnableCollision(true);
		}
	}
}

void ADungeonRoomManager::UnlockDoors()
{
	for (TObjectPtr<AActor> Door : ConnectedDoors)
	{
		if (Door)
		{
			Door->SetActorHiddenInGame(true);
			Door->SetActorEnableCollision(false);
		}
	}
}

void ADungeonRoomManager::SpawnEnemies()
{
	if (!EnemyPrefab) return;

	const int32 NumEnemiesToSpawn = FMath::RandRange(3, 5);
	AliveEnemiesCount = 0;

	for (int32 i = 0; i < NumEnemiesToSpawn; i++)
	{
		FVector SpawnLocation;
		bool bValidPointFound = false;

		for (int32 Tries = 0; Tries < 10 && !bValidPointFound; Tries++)
		{
			const float Padding = 150.0f;
			const FVector Center = GetActorLocation();

			const float RX = FMath::RandRange(Center.X - (RoomSize.X * 0.5f) + Padding, Center.X + (RoomSize.X * 0.5f) - Padding);
			const float RY = FMath::RandRange(Center.Y - (RoomSize.Y * 0.5f) + Padding, Center.Y + (RoomSize.Y * 0.5f) - Padding);

			FHitResult HitResult;
			FCollisionQueryParams TraceParams;
			TraceParams.AddIgnoredActor(this);

			if (GetWorld()->LineTraceSingleByChannel(HitResult, FVector(RX, RY, Center.Z + 2000.f), FVector(RX, RY, Center.Z - 500.f), ECC_WorldStatic, TraceParams))
			{
				SpawnLocation = HitResult.ImpactPoint + FVector(0, 0, 100.0f);
				if (IsSpawnLocationValid(SpawnLocation)) bValidPointFound = true;
			}
		}

		if (bValidPointFound)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

			if (TObjectPtr<AEnemyCharacter> Enemy = GetWorld()->SpawnActor<AEnemyCharacter>(EnemyPrefab, SpawnLocation, FRotator(0, FMath::RandRange(0.f, 360.f), 0), SpawnParams))
			{
				Enemy->ParentRoom = this;
				AliveEnemiesCount++;
			}
		}
	}

	if (AliveEnemiesCount == 0) OnEnemyDied();
}

bool ADungeonRoomManager::IsSpawnLocationValid(FVector Location)
{
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	return !GetWorld()->OverlapAnyTestByChannel(Location, FQuat::Identity, ECC_WorldStatic, FCollisionShape::MakeSphere(45.0f), QueryParams);
}

void ADungeonRoomManager::OnEnemyDied()
{
	AliveEnemiesCount--;

	if (AliveEnemiesCount <= 0 && !bIsCleared)
	{
		bIsCleared = true;
		UnlockDoors();

		// Update HUD
		if (TObjectPtr<APlayerController> PC = GetWorld()->GetFirstPlayerController())
		{
			if (TObjectPtr<AGun_phiriaHUD> PlayerHUD = Cast<AGun_phiriaHUD>(PC->GetHUD()))
			{
				PlayerHUD->ShowMissionClearMessage();
			}
		}
	}
}