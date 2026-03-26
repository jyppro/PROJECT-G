#include "DungeonGenerator.h"
#include "DungeonRoomManager.h"
#include "../Enemy/EnemyCharacter.h"
#include "../Gun_phiriaCharacter.h"
#include "TimerManager.h"

// Engine Headers
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"

// Component Headers
#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"

ADungeonGenerator::ADungeonGenerator()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ADungeonGenerator::BeginPlay()
{
	Super::BeginPlay();

	// 1. 맵이 열리자마자 렉이 걸리기 전에 플레이어 화면을 강제로 까맣게 만듭니다.
	if (AGun_phiriaCharacter* PlayerChar = Cast<AGun_phiriaCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)))
	{
		PlayerChar->ForceBlackScreen();
	}

	// 2. 화면이 까맣게 렌더링될 아주 짧은 시간(0.2초)을 벌어준 뒤에 진짜 생성을 시작합니다.
	FTimerHandle GenerationTimerHandle;
	GetWorld()->GetTimerManager().SetTimer(
		GenerationTimerHandle,
		this,
		&ADungeonGenerator::ExecuteGeneration,
		0.2f,
		false
	);
}

void ADungeonGenerator::ExecuteGeneration()
{
	// =========================================================
	// 원래 BeginPlay에 있던 무거운 맵 생성 로직들을 여기서 실행합니다.
	// 화면이 까맣기 때문에 유저는 프레임 드랍을 전혀 느끼지 못합니다.
	// =========================================================
	GenerateRandomRooms();
	SeparateRooms();
	SelectMainRooms();
	ConnectMainRooms();
	CreateCorridors();
	SpawnDungeonActors();
	TeleportPlayerToRandomRoom();
	SetupRoomManagers();
	SpawnItemsInRooms();

	if (bShowDebugBoxes) DrawDebugRooms();

	// 3. 모든 생성이 끝났습니다! 이제 화면을 서서히 밝힙니다.
	if (AGun_phiriaCharacter* PlayerChar = Cast<AGun_phiriaCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)))
	{
		PlayerChar->StartFadeIn(2.0f); // 2초 동안 페이드 인
	}
}

// --- Logic Helpers ---

float ADungeonGenerator::SnapToGrid(float Value)
{
	return FMath::RoundHalfToEven(Value / GridSize) * GridSize;
}

bool ADungeonGenerator::DoRoomsOverlap(const FDungeonRoom& A, const FDungeonRoom& B)
{
	const float Pad = RoomPadding * 0.5f;
	const FBox2D BoxA(
		FVector2D(A.CenterLocation.X - (A.Size.X * 0.5f) - Pad, A.CenterLocation.Y - (A.Size.Y * 0.5f) - Pad),
		FVector2D(A.CenterLocation.X + (A.Size.X * 0.5f) + Pad, A.CenterLocation.Y + (A.Size.Y * 0.5f) + Pad)
	);
	const FBox2D BoxB(
		FVector2D(B.CenterLocation.X - (B.Size.X * 0.5f) - Pad, B.CenterLocation.Y - (B.Size.Y * 0.5f) - Pad),
		FVector2D(B.CenterLocation.X + (B.Size.X * 0.5f) + Pad, B.CenterLocation.Y + (B.Size.Y * 0.5f) + Pad)
	);

	return BoxA.Intersect(BoxB);
}

// --- Generation Logic ---

void ADungeonGenerator::GenerateRandomRooms()
{
	RoomList.Empty();
	if (MainRoomPrefabs.IsEmpty()) return;

	float TotalWeight = 0.0f;
	for (const auto& Prefab : MainRoomPrefabs) TotalWeight += Prefab.SpawnWeight;

	for (int32 i = 0; i < NumberOfRoomsToGenerate; ++i)
	{
		float RandomValue = FMath::FRandRange(0.0f, TotalWeight);
		float AccumulatedWeight = 0.0f;
		int32 SelectedIndex = 0;

		for (int32 j = 0; j < MainRoomPrefabs.Num(); ++j)
		{
			AccumulatedWeight += MainRoomPrefabs[j].SpawnWeight;
			if (RandomValue <= AccumulatedWeight) { SelectedIndex = j; break; }
		}

		FDungeonRoom NewRoom;
		NewRoom.SelectedPrefabIndex = SelectedIndex;
		NewRoom.Size = MainRoomPrefabs[SelectedIndex].Size;

		const FVector2D RandomPoint = FMath::RandPointInCircle(SpawnRadius);
		NewRoom.CenterLocation = FVector(SnapToGrid(RandomPoint.X), SnapToGrid(RandomPoint.Y), 0.0f);
		RoomList.Add(NewRoom);
	}
}

void ADungeonGenerator::SeparateRooms()
{
	bool bHasOverlaps = true;
	int32 Iterations = 0;

	while (bHasOverlaps && Iterations++ < 1000)
	{
		bHasOverlaps = false;
		for (int i = 0; i < RoomList.Num(); i++)
		{
			for (int j = i + 1; j < RoomList.Num(); j++)
			{
				if (DoRoomsOverlap(RoomList[i], RoomList[j]))
				{
					bHasOverlaps = true;
					FVector Dir = RoomList[i].CenterLocation - RoomList[j].CenterLocation;
					if (Dir.IsNearlyZero()) Dir = FVector(FMath::RandRange(-1.f, 1.f), FMath::RandRange(-1.f, 1.f), 0.f);
					Dir.Normalize();

					if (FMath::Abs(Dir.X) > FMath::Abs(Dir.Y))
					{
						RoomList[i].CenterLocation.X += FMath::Sign(Dir.X) * GridSize;
						RoomList[j].CenterLocation.X -= FMath::Sign(Dir.X) * GridSize;
					}
					else
					{
						RoomList[i].CenterLocation.Y += FMath::Sign(Dir.Y) * GridSize;
						RoomList[j].CenterLocation.Y -= FMath::Sign(Dir.Y) * GridSize;
					}
				}
			}
		}
	}
}

void ADungeonGenerator::SelectMainRooms()
{
	for (auto& Room : RoomList) Room.bIsMainRoom = (Room.SelectedPrefabIndex != -1);
}

void ADungeonGenerator::ConnectMainRooms()
{
	FinalPaths.Empty();
	TArray<int32> MainRoomIndices;
	for (int32 i = 0; i < RoomList.Num(); i++) if (RoomList[i].bIsMainRoom) MainRoomIndices.Add(i);

	if (MainRoomIndices.Num() == 0) return;

	TArray<int32> Reached = { MainRoomIndices[0] };
	TArray<int32> Unreached = MainRoomIndices;
	Unreached.RemoveAt(0);

	TArray<FRoomEdge> ExtraPaths;

	while (Unreached.Num() > 0)
	{
		float MinDist = MAX_flt;
		int32 R_Idx = -1, U_Idx = -1;

		for (int32 r : Reached)
		{
			for (int32 u : Unreached)
			{
				float Dist = FVector::Dist(RoomList[r].CenterLocation, RoomList[u].CenterLocation);
				ExtraPaths.AddUnique({ r, u, Dist });
				if (Dist < MinDist) { MinDist = Dist; R_Idx = r; U_Idx = u; }
			}
		}

		FinalPaths.Add({ R_Idx, U_Idx, MinDist });
		Reached.Add(U_Idx);
		Unreached.Remove(U_Idx);
	}

	for (const auto& Edge : ExtraPaths)
	{
		if (!FinalPaths.Contains(Edge) && FMath::FRand() <= AdditionalPathProbability) FinalPaths.Add(Edge);
	}
}

void ADungeonGenerator::CreateCorridors()
{
	CorridorTiles.Empty();

	for (const auto& Edge : FinalPaths)
	{
		FVector A = RoomList[Edge.RoomAIndex].CenterLocation;
		FVector B = RoomList[Edge.RoomBIndex].CenterLocation;

		auto AddTile = [&](FVector P) { if (!IsPointInAnyMainRoom(P)) CorridorTiles.AddUnique(P); };

		auto DrawLine = [&](FVector Start, FVector End)
			{
				FVector Curr = Start;
				while (!FMath::IsNearlyEqual(Curr.X, End.X, 10.f))
				{
					Curr.X += FMath::Sign(End.X - Curr.X) * FMath::Min(CorridorWidth, FMath::Abs(End.X - Curr.X));
					AddTile(Curr);
				}
				while (!FMath::IsNearlyEqual(Curr.Y, End.Y, 10.f))
				{
					Curr.Y += FMath::Sign(End.Y - Curr.Y) * FMath::Min(CorridorWidth, FMath::Abs(End.Y - Curr.Y));
					AddTile(Curr);
				}
			};

		if (FMath::Abs(B.X - A.X) > FMath::Abs(B.Y - A.Y))
		{
			float MidX = (A.X + B.X) * 0.5f;
			DrawLine(A, FVector(MidX, A.Y, 0));
			DrawLine(B, FVector(MidX, B.Y, 0));
			DrawLine(FVector(MidX, A.Y, 0), FVector(MidX, B.Y, 0));
		}
		else
		{
			float MidY = (A.Y + B.Y) * 0.5f;
			DrawLine(A, FVector(A.X, MidY, 0));
			DrawLine(B, FVector(B.X, MidY, 0));
			DrawLine(FVector(A.X, MidY, 0), FVector(B.X, MidY, 0));
		}
	}
}

void ADungeonGenerator::SpawnDungeonActors()
{
	if (MainRoomPrefabs.IsEmpty()) return;

	// 방 스폰
	for (const auto& Room : RoomList)
	{
		if (Room.bIsMainRoom && Room.SelectedPrefabIndex >= 0)
		{
			GetWorld()->SpawnActor<AActor>(MainRoomPrefabs[Room.SelectedPrefabIndex].RoomClass, Room.CenterLocation, FRotator::ZeroRotator);
		}
	}

	// 복도 바닥 스폰
	for (const FVector& Tile : CorridorTiles)
	{
		if (TObjectPtr<AActor> Corridor = GetWorld()->SpawnActor<AActor>(CorridorPrefab, Tile, FRotator::ZeroRotator))
		{
			Corridor->SetActorScale3D(FVector(CorridorWidth / 100.f, CorridorWidth / 100.f, 1.f));
		}
	}

	// 복도 벽 스폰
	auto HasAdjacentCorridor = [&](FVector CheckPos) -> bool
		{
			for (const FVector& Tile : CorridorTiles)
			{
				if (FVector::Dist(Tile, CheckPos) < CorridorWidth * 0.9f) return true;
			}
			return false;
		};

	for (const FVector& TilePos : CorridorTiles)
	{
		if (!CorridorPrefab) continue;

		// 4방향 위치 계산
		TArray<FVector> Directions = {
			TilePos + FVector(0.0f, CorridorWidth, 0.0f),  // North
			TilePos + FVector(0.0f, -CorridorWidth, 0.0f), // South
			TilePos + FVector(CorridorWidth, 0.0f, 0.0f),  // East
			TilePos + FVector(-CorridorWidth, 0.0f, 0.0f)  // West
		};

		const float WallHeight = 500.0f;
		const float WallThickness = 50.0f;

		for (int32 i = 0; i < Directions.Num(); ++i)
		{
			FVector CheckPos = Directions[i];
			if (!HasAdjacentCorridor(CheckPos) && !IsPointInAnyMainRoom(CheckPos))
			{
				FVector Offset = (Directions[i] - TilePos) * 0.5f;
				FVector WallPos = TilePos + Offset + FVector(0.0f, 0.0f, WallHeight * 0.5f);

				if (TObjectPtr<AActor> Wall = GetWorld()->SpawnActor<AActor>(CorridorPrefab, WallPos, FRotator::ZeroRotator))
				{
					// 방향에 따른 스케일 조절 (i < 2는 남북 방향, i >= 2는 동서 방향)
					FVector WallScale = (i < 2) ?
						FVector(CorridorWidth / 100.f, WallThickness / 100.f, WallHeight / 100.f) :
						FVector(WallThickness / 100.f, CorridorWidth / 100.f, WallHeight / 100.f);

					Wall->SetActorScale3D(WallScale);
				}
			}
		}
	}

	// 문 생성
	FCollisionObjectQueryParams ObjectParams = FCollisionObjectQueryParams::AllObjects;
	for (const FVector& Tile : CorridorTiles)
	{
		TArray<FOverlapResult> Overlaps;
		if (GetWorld()->OverlapMultiByObjectType(Overlaps, Tile + FVector(0, 0, 150.f), FQuat::Identity, ObjectParams, FCollisionShape::MakeSphere(CorridorWidth * 0.6f)))
		{
			for (const auto& Result : Overlaps)
			{
				if (Result.GetComponent() && Result.GetComponent()->ComponentHasTag("FakeWall"))
				{
					FTransform DoorTr = Result.GetComponent()->GetComponentTransform();
					if (TObjectPtr<AActor> Door = GetWorld()->SpawnActor<AActor>(DoorPrefab, DoorTr))
					{
						// 문의 크기를 원래 FakeWall의 크기로 설정합니다.
						Door->SetActorScale3D(DoorTr.GetScale3D());

						Door->SetActorHiddenInGame(true);
						Door->SetActorEnableCollision(false);
						SpawnedDoors.Add(Door);
					}
					Result.GetComponent()->DestroyComponent();
				}
			}
		}
	}
}

void ADungeonGenerator::SetupRoomManagers()
{
	if (!RoomManagerPrefab) return;

	for (int32 i = 0; i < RoomList.Num(); i++)
	{
		const auto& Room = RoomList[i];
		if (!Room.bIsMainRoom || i == PlayerSpawnRoomIndex) continue;

		if (TObjectPtr<ADungeonRoomManager> Manager = GetWorld()->SpawnActor<ADungeonRoomManager>(RoomManagerPrefab, Room.CenterLocation + FVector(0, 0, 200.f), FRotator::ZeroRotator))
		{
			Manager->RoomSize = FVector(Room.Size.X, Room.Size.Y, 1000.f);
			Manager->EnemyPrefab = EnemyPrefab;

			if (Manager->RoomTrigger)
				Manager->RoomTrigger->SetBoxExtent(FVector((Room.Size.X * 0.5f) - 150.f, (Room.Size.Y * 0.5f) - 150.f, 500.f));

			for (TObjectPtr<AActor> Door : SpawnedDoors)
			{
				if (!IsValid(Door)) continue;
				FVector Loc = Door->GetActorLocation();
				if (Loc.X >= Room.CenterLocation.X - (Room.Size.X * 0.5f) - CorridorWidth && Loc.X <= Room.CenterLocation.X + (Room.Size.X * 0.5f) + CorridorWidth &&
					Loc.Y >= Room.CenterLocation.Y - (Room.Size.Y * 0.5f) - CorridorWidth && Loc.Y <= Room.CenterLocation.Y + (Room.Size.Y * 0.5f) + CorridorWidth)
				{
					Manager->ConnectedDoors.Add(Door);
				}
			}
		}
	}
}

void ADungeonGenerator::SpawnItemsInRooms()
{
	if (ItemPrefabs.IsEmpty()) return;

	for (const auto& Room : RoomList)
	{
		if (!Room.bIsMainRoom) continue;

		int32 Count = FMath::RandRange(MinItemsPerRoom, MaxItemsPerRoom);
		for (int32 j = 0; j < Count; j++)
		{
			FVector SpawnLoc;
			bool bFound = false;
			for (int32 Tries = 0; Tries < 10 && !bFound; Tries++)
			{
				float RX = FMath::RandRange(Room.CenterLocation.X - Room.Size.X * 0.4f, Room.CenterLocation.X + Room.Size.X * 0.4f);
				float RY = FMath::RandRange(Room.CenterLocation.Y - Room.Size.Y * 0.4f, Room.CenterLocation.Y + Room.Size.Y * 0.4f);

				FHitResult Hit;
				if (GetWorld()->LineTraceSingleByChannel(Hit, FVector(RX, RY, 2000), FVector(RX, RY, -500), ECC_WorldStatic))
				{
					SpawnLoc = Hit.ImpactPoint + FVector(0, 0, 50.f);
					if (!GetWorld()->OverlapAnyTestByChannel(SpawnLoc, FQuat::Identity, ECC_WorldStatic, FCollisionShape::MakeSphere(20.f))) bFound = true;
				}
			}
			if (bFound) GetWorld()->SpawnActor<AActor>(ItemPrefabs[FMath::RandRange(0, ItemPrefabs.Num() - 1)], SpawnLoc, FRotator(0, FMath::RandRange(0.f, 360.f), 0));
		}
	}
}

// --- Helper Implementation ---

bool ADungeonGenerator::IsPointInAnyMainRoom(FVector Point)
{
	for (const auto& Room : RoomList)
	{
		if (!Room.bIsMainRoom) continue;
		if (FMath::Abs(Point.X - Room.CenterLocation.X) < (Room.Size.X * 0.5f - 10.f) &&
			FMath::Abs(Point.Y - Room.CenterLocation.Y) < (Room.Size.Y * 0.5f - 10.f)) return true;
	}
	return false;
}

void ADungeonGenerator::TeleportPlayerToRandomRoom()
{
	TArray<int32> Mains;
	for (int32 i = 0; i < RoomList.Num(); i++) if (RoomList[i].bIsMainRoom) Mains.Add(i);
	if (Mains.IsEmpty()) return;

	PlayerSpawnRoomIndex = Mains[FMath::RandRange(0, Mains.Num() - 1)];
	if (APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
		Player->TeleportTo(RoomList[PlayerSpawnRoomIndex].CenterLocation + FVector(0, 0, 1000.f), Player->GetActorRotation());
}

void ADungeonGenerator::DrawDebugRooms()
{
#if !UE_BUILD_SHIPPING
	FlushPersistentDebugLines(GetWorld());
	for (const auto& Room : RoomList)
		DrawDebugBox(GetWorld(), Room.CenterLocation, FVector(Room.Size.X * 0.5f, Room.Size.Y * 0.5f, 50), Room.bIsMainRoom ? FColor::Green : FColor::Red, true, -1, 0, Room.bIsMainRoom ? 20 : 5);
#endif
}