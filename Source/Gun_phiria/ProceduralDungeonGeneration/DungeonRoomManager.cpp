#include "DungeonRoomManager.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "../Enemy/EnemyCharacter.h" 

ADungeonRoomManager::ADungeonRoomManager()
{
	PrimaryActorTick.bCanEverTick = false;

	RoomTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("RoomTrigger"));
	RootComponent = RoomTrigger;

	// 오직 플레이어의 겹침만 감지하도록 설정
	RoomTrigger->SetCollisionProfileName(TEXT("Trigger"));
}

void ADungeonRoomManager::BeginPlay()
{
	Super::BeginPlay();
	RoomTrigger->OnComponentBeginOverlap.AddDynamic(this, &ADungeonRoomManager::OnPlayerEnter);
}

// 플레이어가 방 중앙 박스에 닿았을 때!
void ADungeonRoomManager::OnPlayerEnter(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bIsTriggered || bIsCleared) return;

	ACharacter* PlayerChar = Cast<ACharacter>(OtherActor);
	// 닿은 사람이 플레이어인지 확인
	if (PlayerChar && PlayerChar == UGameplayStatics::GetPlayerCharacter(GetWorld(), 0))
	{
		bIsTriggered = true; // 중복 트리거 방지

		LockDoors(); // 1. 방 문을 닫아버림
		SpawnEnemies(); // 2. 적 소환 시작
	}
}

void ADungeonRoomManager::LockDoors()
{
	for (AActor* Door : ConnectedDoors)
	{
		if (Door)
		{
			// 문을 보이게 하고, 충돌을 켜서 길을 막음 (배틀그라운드 레드존처럼 가둠)
			Door->SetActorHiddenInGame(false);
			Door->SetActorEnableCollision(true);
		}
	}
}

void ADungeonRoomManager::UnlockDoors()
{
	for (AActor* Door : ConnectedDoors)
	{
		if (Door)
		{
			// 문을 투명하게 하고, 충돌을 꺼서 지나갈 수 있게 함 (열림)
			Door->SetActorHiddenInGame(true);
			Door->SetActorEnableCollision(false);
		}
	}
}

void ADungeonRoomManager::SpawnEnemies()
{
	if (!EnemyPrefab) return;

	// 나중에 변수로 빼서 방 난이도별로 조절할 수 있습니다.
	int32 NumEnemiesToSpawn = FMath::RandRange(3, 5);
	AliveEnemiesCount = 0;

	for (int32 j = 0; j < NumEnemiesToSpawn; j++)
	{
		FVector SpawnLocation;
		bool bValidPointFound = false;
		int32 MaxTries = 10;
		int32 CurrentTries = 0;

		while (!bValidPointFound && CurrentTries < MaxTries)
		{
			CurrentTries++;

			float Padding = 150.0f;
			FVector Center = GetActorLocation();
			float MinX = Center.X - (RoomSize.X * 0.5f) + Padding;
			float MaxX = Center.X + (RoomSize.X * 0.5f) - Padding;
			float MinY = Center.Y - (RoomSize.Y * 0.5f) + Padding;
			float MaxY = Center.Y + (RoomSize.Y * 0.5f) - Padding;

			float RandomX = FMath::RandRange(MinX, MaxX);
			float RandomY = FMath::RandRange(MinY, MaxY);

			// 하늘에서 수직 레이저 발사 (이전에 완성한 완벽한 스폰 로직)
			FVector TraceStart = FVector(RandomX, RandomY, Center.Z + 2000.0f);
			FVector TraceEnd = FVector(RandomX, RandomY, Center.Z - 500.0f);

			FHitResult HitResult;
			FCollisionQueryParams TraceParams;
			TraceParams.AddIgnoredActor(this);

			if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic, TraceParams))
			{
				SpawnLocation = HitResult.ImpactPoint + FVector(0.0f, 0.0f, 100.0f);
				if (IsSpawnLocationValid(SpawnLocation))
				{
					bValidPointFound = true;
				}
			}
		}

		if (bValidPointFound)
		{
			FRotator SpawnRotation = FRotator(0.0f, FMath::RandRange(0.0f, 360.0f), 0.0f);
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

			AEnemyCharacter* SpawnedEnemy = GetWorld()->SpawnActor<AEnemyCharacter>(EnemyPrefab, SpawnLocation, SpawnRotation, SpawnParams);

			if (SpawnedEnemy)
			{
				// ★ 핵심: 스폰된 적에게 이 방(RoomManager)이 네 주인이라고 알려줌!
				SpawnedEnemy->ParentRoom = this;
				AliveEnemiesCount++;
			}
		}
	}

	// 만약 스폰에 모두 실패해서 적이 0마리라면 즉시 클리어 처리
	if (AliveEnemiesCount == 0)
	{
		OnEnemyDied();
	}
}

bool ADungeonRoomManager::IsSpawnLocationValid(FVector Location)
{
	float CheckRadius = 45.0f;
	FCollisionShape CheckSphere = FCollisionShape::MakeSphere(CheckRadius);
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	bool bIsOverlapping = GetWorld()->OverlapAnyTestByChannel(Location, FQuat::Identity, ECC_WorldStatic, CheckSphere, QueryParams);

	return !bIsOverlapping; // 겹치는 게 없으면 스폰 성공
}

void ADungeonRoomManager::OnEnemyDied()
{
	AliveEnemiesCount--;

	// 모든 적 처치 완료!
	if (AliveEnemiesCount <= 0 && !bIsCleared)
	{
		bIsCleared = true;
		UnlockDoors(); // 다시 문이 스르륵 열립니다.

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("MISSION CLEAR! Doors Opened."));
		}

		// 맵에 흩어진 아이템 드랍, 다음 방 오픈 효과 등 추후 확장 가능!
	}
}