#include "DungeonGenerator.h"
#include "DrawDebugHelpers.h" 
#include "Math/UnrealMathUtility.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/OverlapResult.h"
#include "Kismet/GameplayStatics.h"
#include "../Enemy/EnemyCharacter.h"
#include "CollisionQueryParams.h"
#include "WorldCollision.h"
#include "Engine/World.h"

ADungeonGenerator::ADungeonGenerator()
{
	PrimaryActorTick.bCanEverTick = false;
	PlayerSpawnRoomIndex = -1;
}

void ADungeonGenerator::BeginPlay()
{
	Super::BeginPlay();

	GenerateRandomRooms();
	SeparateRooms();
	SelectMainRooms();
	ConnectMainRooms();
	CreateCorridors();
	SpawnDungeonActors();
	TeleportPlayerToRandomRoom();

	SpawnEnemies();
	RemoveStartingRoomDoors();

	if (bShowDebugBoxes)
	{
		DrawDebugRooms();
	}
}

float ADungeonGenerator::SnapToGrid(float Value)
{
	return FMath::RoundHalfToEven(Value / GridSize) * GridSize;
}

void ADungeonGenerator::GenerateRandomRooms()
{
	RoomList.Empty();

	if (MainRoomPrefabs.IsEmpty()) return;

	float TotalWeight = 0.0f;
	for (const FRoomPrefabData& PrefabData : MainRoomPrefabs)
	{
		TotalWeight += PrefabData.SpawnWeight;
	}

	for (int32 i = 0; i < NumberOfRoomsToGenerate; ++i)
	{
		FDungeonRoom NewRoom;

		float RandomValue = FMath::FRandRange(0.0f, TotalWeight);
		float AccumulatedWeight = 0.0f;
		int32 SelectedIndex = 0;

		for (int32 j = 0; j < MainRoomPrefabs.Num(); ++j)
		{
			AccumulatedWeight += MainRoomPrefabs[j].SpawnWeight;
			if (RandomValue <= AccumulatedWeight)
			{
				SelectedIndex = j;
				break;
			}
		}

		NewRoom.SelectedPrefabIndex = SelectedIndex;
		NewRoom.Size = MainRoomPrefabs[SelectedIndex].Size;

		FVector2D RandomPoint = FMath::RandPointInCircle(SpawnRadius);
		NewRoom.CenterLocation = FVector(SnapToGrid(RandomPoint.X), SnapToGrid(RandomPoint.Y), 0.0f);

		RoomList.Add(NewRoom);
	}
}

bool ADungeonGenerator::DoRoomsOverlap(const FDungeonRoom& A, const FDungeonRoom& B)
{
	float Pad = RoomPadding * 0.5f;

	float LeftA = A.CenterLocation.X - (A.Size.X * 0.5f) - Pad;
	float RightA = A.CenterLocation.X + (A.Size.X * 0.5f) + Pad;
	float TopA = A.CenterLocation.Y - (A.Size.Y * 0.5f) - Pad;
	float BottomA = A.CenterLocation.Y + (A.Size.Y * 0.5f) + Pad;

	float LeftB = B.CenterLocation.X - (B.Size.X * 0.5f) - Pad;
	float RightB = B.CenterLocation.X + (B.Size.X * 0.5f) + Pad;
	float TopB = B.CenterLocation.Y - (B.Size.Y * 0.5f) - Pad;
	float BottomB = B.CenterLocation.Y + (B.Size.Y * 0.5f) + Pad;

	if (RightA <= LeftB || LeftA >= RightB || BottomA <= TopB || TopA >= BottomB)
	{
		return false;
	}
	return true;
}

void ADungeonGenerator::SeparateRooms()
{
	bool bHasOverlaps = true;
	int32 MaxIterations = 1000;
	int32 CurrentIteration = 0;

	while (bHasOverlaps && CurrentIteration < MaxIterations)
	{
		bHasOverlaps = false;
		CurrentIteration++;

		for (int i = 0; i < RoomList.Num(); i++)
		{
			for (int j = i + 1; j < RoomList.Num(); j++)
			{
				FDungeonRoom& RoomA = RoomList[i];
				FDungeonRoom& RoomB = RoomList[j];

				if (DoRoomsOverlap(RoomA, RoomB))
				{
					bHasOverlaps = true;
					FVector Direction = RoomA.CenterLocation - RoomB.CenterLocation;

					if (Direction.IsNearlyZero())
					{
						Direction = FVector(FMath::RandRange(-1.f, 1.f), FMath::RandRange(-1.f, 1.f), 0.f);
					}

					Direction.Normalize();

					if (FMath::Abs(Direction.X) > FMath::Abs(Direction.Y))
					{
						RoomA.CenterLocation.X += FMath::Sign(Direction.X) * GridSize;
						RoomB.CenterLocation.X -= FMath::Sign(Direction.X) * GridSize;
					}
					else
					{
						RoomA.CenterLocation.Y += FMath::Sign(Direction.Y) * GridSize;
						RoomB.CenterLocation.Y -= FMath::Sign(Direction.Y) * GridSize;
					}
				}
			}
		}
	}
}

void ADungeonGenerator::SelectMainRooms()
{
	for (FDungeonRoom& Room : RoomList)
	{
		Room.bIsMainRoom = (Room.SelectedPrefabIndex != -1);
	}
}

void ADungeonGenerator::ConnectMainRooms()
{
	FinalPaths.Empty();

	TArray<int32> MainRoomIndices;
	for (int32 i = 0; i < RoomList.Num(); i++)
	{
		if (RoomList[i].bIsMainRoom)
		{
			MainRoomIndices.Add(i);
		}
	}

	if (MainRoomIndices.Num() == 0) return;

	TArray<int32> ReachedRooms;
	TArray<int32> UnreachedRooms = MainRoomIndices;

	ReachedRooms.Add(UnreachedRooms[0]);
	UnreachedRooms.RemoveAt(0);

	TArray<FRoomEdge> PotentialExtraPaths;

	while (UnreachedRooms.Num() > 0)
	{
		float MinDistance = MAX_flt;
		int32 ReachedNodeIndex = -1;
		int32 UnreachedNodeIndex = -1;

		for (int32 R_Idx : ReachedRooms)
		{
			for (int32 U_Idx : UnreachedRooms)
			{
				float Distance = FVector::Dist(RoomList[R_Idx].CenterLocation, RoomList[U_Idx].CenterLocation);

				FRoomEdge PotentialEdge;
				PotentialEdge.RoomAIndex = R_Idx;
				PotentialEdge.RoomBIndex = U_Idx;
				PotentialEdge.Distance = Distance;
				PotentialExtraPaths.AddUnique(PotentialEdge);

				if (Distance < MinDistance)
				{
					MinDistance = Distance;
					ReachedNodeIndex = R_Idx;
					UnreachedNodeIndex = U_Idx;
				}
			}
		}

		FRoomEdge NewMSTEdge;
		NewMSTEdge.RoomAIndex = ReachedNodeIndex;
		NewMSTEdge.RoomBIndex = UnreachedNodeIndex;
		NewMSTEdge.Distance = MinDistance;
		FinalPaths.Add(NewMSTEdge);

		ReachedRooms.Add(UnreachedNodeIndex);
		UnreachedRooms.Remove(UnreachedNodeIndex);
	}

	for (const FRoomEdge& ExtraEdge : PotentialExtraPaths)
	{
		if (FinalPaths.Contains(ExtraEdge)) continue;

		if (FMath::RandRange(0.0f, 1.0f) <= AdditionalPathProbability)
		{
			FinalPaths.Add(ExtraEdge);
		}
	}
}

void ADungeonGenerator::CreateCorridors()
{
	CorridorTiles.Empty();

	for (const FRoomEdge& Edge : FinalPaths)
	{
		FVector PosA = RoomList[Edge.RoomAIndex].CenterLocation;
		FVector PosB = RoomList[Edge.RoomBIndex].CenterLocation;

		float dX = PosB.X - PosA.X;
		float dY = PosB.Y - PosA.Y;

		auto DrawLine = [&](FVector Start, FVector End)
			{
				FVector Current = Start;

				while (!FMath::IsNearlyEqual(Current.X, End.X, 10.0f))
				{
					float MoveAmount = FMath::Min(CorridorWidth, FMath::Abs(End.X - Current.X));
					Current.X += FMath::Sign(End.X - Current.X) * MoveAmount;
					if (!IsPointInAnyMainRoom(Current)) CorridorTiles.AddUnique(Current);
				}
				while (!FMath::IsNearlyEqual(Current.Y, End.Y, 10.0f))
				{
					float MoveAmount = FMath::Min(CorridorWidth, FMath::Abs(End.Y - Current.Y));
					Current.Y += FMath::Sign(End.Y - Current.Y) * MoveAmount;
					if (!IsPointInAnyMainRoom(Current)) CorridorTiles.AddUnique(Current);
				}
			};

		if (FMath::Abs(dX) > FMath::Abs(dY))
		{
			float MidX = (PosA.X + PosB.X) / 2.0f;
			DrawLine(PosA, FVector(MidX, PosA.Y, 0));
			DrawLine(PosB, FVector(MidX, PosB.Y, 0));
			DrawLine(FVector(MidX, PosA.Y, 0), FVector(MidX, PosB.Y, 0));
		}
		else
		{
			float MidY = (PosA.Y + PosB.Y) / 2.0f;
			DrawLine(PosA, FVector(PosA.X, MidY, 0));
			DrawLine(PosB, FVector(PosB.X, MidY, 0));
			DrawLine(FVector(PosA.X, MidY, 0), FVector(PosB.X, MidY, 0));
		}
	}
}

void ADungeonGenerator::SpawnDungeonActors()
{
	if (MainRoomPrefabs.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Room Prefabs are not assigned!"));
		return;
	}

	// 방 스폰 로직
	for (const FDungeonRoom& Room : RoomList)
	{
		if (Room.bIsMainRoom && Room.SelectedPrefabIndex >= 0)
		{
			FTransform SpawnTransform;
			SpawnTransform.SetLocation(FVector(Room.CenterLocation.X, Room.CenterLocation.Y, 0.0f));

			TSubclassOf<AActor> SelectedRoomPrefab = MainRoomPrefabs[Room.SelectedPrefabIndex].RoomClass;

			if (SelectedRoomPrefab)
			{
				GetWorld()->SpawnActor<AActor>(SelectedRoomPrefab, SpawnTransform);
			}
		}
	}

	// 복도 바닥 스폰 로직
	for (const FVector& TilePos : CorridorTiles)
	{
		if (CorridorPrefab)
		{
			FVector SpawnPos = FVector(TilePos.X, TilePos.Y, 0.0f);
			AActor* SpawnedCorridor = GetWorld()->SpawnActor<AActor>(CorridorPrefab, SpawnPos, FRotator::ZeroRotator);

			if (SpawnedCorridor)
			{
				FVector NewScale = FVector(CorridorWidth / 100.0f, CorridorWidth / 100.0f, 1.0f);
				SpawnedCorridor->SetActorScale3D(NewScale);
			}
		}
	}

	// 복도 양옆 벽 스폰 로직
	auto HasAdjacentCorridor = [&](FVector CheckPos) -> bool
		{
			for (const FVector& Tile : CorridorTiles)
			{
				if (FVector::Dist(Tile, CheckPos) < CorridorWidth * 0.9f)
				{
					return true;
				}
			}
			return false;
		};

	for (const FVector& TilePos : CorridorTiles)
	{
		if (!CorridorPrefab) continue;

		FVector NorthPos = TilePos + FVector(0.0f, CorridorWidth, 0.0f);
		FVector SouthPos = TilePos + FVector(0.0f, -CorridorWidth, 0.0f);
		FVector EastPos = TilePos + FVector(CorridorWidth, 0.0f, 0.0f);
		FVector WestPos = TilePos + FVector(-CorridorWidth, 0.0f, 0.0f);

		float WallThickness = 50.0f;
		float WallHeight = 500.0f;

		if (!HasAdjacentCorridor(NorthPos) && !IsPointInAnyMainRoom(NorthPos))
		{
			FVector WallPos = TilePos + FVector(0.0f, CorridorWidth * 0.5f, WallHeight * 0.5f);
			AActor* Wall = GetWorld()->SpawnActor<AActor>(CorridorPrefab, WallPos, FRotator::ZeroRotator);
			if (Wall) Wall->SetActorScale3D(FVector(CorridorWidth / 100.0f, WallThickness / 100.0f, WallHeight / 100.0f));
		}
		if (!HasAdjacentCorridor(SouthPos) && !IsPointInAnyMainRoom(SouthPos))
		{
			FVector WallPos = TilePos + FVector(0.0f, -CorridorWidth * 0.5f, WallHeight * 0.5f);
			AActor* Wall = GetWorld()->SpawnActor<AActor>(CorridorPrefab, WallPos, FRotator::ZeroRotator);
			if (Wall) Wall->SetActorScale3D(FVector(CorridorWidth / 100.0f, WallThickness / 100.0f, WallHeight / 100.0f));
		}
		if (!HasAdjacentCorridor(EastPos) && !IsPointInAnyMainRoom(EastPos))
		{
			FVector WallPos = TilePos + FVector(CorridorWidth * 0.5f, 0.0f, WallHeight * 0.5f);
			AActor* Wall = GetWorld()->SpawnActor<AActor>(CorridorPrefab, WallPos, FRotator::ZeroRotator);
			if (Wall) Wall->SetActorScale3D(FVector(WallThickness / 100.0f, CorridorWidth / 100.0f, WallHeight / 100.0f));
		}
		if (!HasAdjacentCorridor(WestPos) && !IsPointInAnyMainRoom(WestPos))
		{
			FVector WallPos = TilePos + FVector(-CorridorWidth * 0.5f, 0.0f, WallHeight * 0.5f);
			AActor* Wall = GetWorld()->SpawnActor<AActor>(CorridorPrefab, WallPos, FRotator::ZeroRotator);
			if (Wall) Wall->SetActorScale3D(FVector(WallThickness / 100.0f, CorridorWidth / 100.0f, WallHeight / 100.0f));
		}
	}

	// 가짜 벽 파괴 및 문(Door) 생성 로직
	for (const FVector& TilePos : CorridorTiles)
	{
		float CheckRadius = CorridorWidth * 0.6f;
		FCollisionShape CheckSphere = FCollisionShape::MakeSphere(CheckRadius);
		FVector CheckPos = FVector(TilePos.X, TilePos.Y, 150.0f);

		TArray<FOverlapResult> OverlapResults;
		FCollisionObjectQueryParams ObjectQueryParams = FCollisionObjectQueryParams::AllObjects;

		bool bHit = GetWorld()->OverlapMultiByObjectType(
			OverlapResults,
			CheckPos,
			FQuat::Identity,
			ObjectQueryParams,
			CheckSphere
		);

		if (bHit)
		{
			for (const FOverlapResult& Result : OverlapResults)
			{
				UPrimitiveComponent* OverlappedComp = Result.GetComponent();

				if (OverlappedComp && OverlappedComp->ComponentHasTag(FName("FakeWall")))
				{
					FTransform DoorTransform = OverlappedComp->GetComponentTransform();

					if (DoorPrefab)
					{
						AActor* SpawnedDoor = GetWorld()->SpawnActor<AActor>(DoorPrefab, DoorTransform);
						if (SpawnedDoor)
						{
							SpawnedDoor->SetActorScale3D(DoorTransform.GetScale3D());

							// 나중에 지울 수 있도록 배열에 기억
							SpawnedDoors.Add(SpawnedDoor);
						}
					}
					OverlappedComp->DestroyComponent();
				}
			}
		}
	}
}

bool ADungeonGenerator::IsPointInAnyMainRoom(FVector Point)
{
	for (const FDungeonRoom& Room : RoomList)
	{
		if (!Room.bIsMainRoom) continue;

		float Left = Room.CenterLocation.X - (Room.Size.X * 0.5f) + 10.0f;
		float Right = Room.CenterLocation.X + (Room.Size.X * 0.5f) - 10.0f;
		float Top = Room.CenterLocation.Y - (Room.Size.Y * 0.5f) + 10.0f;
		float Bottom = Room.CenterLocation.Y + (Room.Size.Y * 0.5f) - 10.0f;

		if (Point.X > Left && Point.X < Right && Point.Y > Top && Point.Y < Bottom)
		{
			return true;
		}
	}
	return false;
}

void ADungeonGenerator::TeleportPlayerToRandomRoom()
{
	// 기존의 복사본 배열 대신, 실제 RoomList의 인덱스들을 저장
	TArray<int32> AvailableMainRoomIndices;
	for (int32 i = 0; i < RoomList.Num(); i++)
	{
		if (RoomList[i].bIsMainRoom)
		{
			AvailableMainRoomIndices.Add(i);
		}
	}

	if (AvailableMainRoomIndices.IsEmpty()) return;

	// 인덱스 배열에서 랜덤으로 하나를 고름
	int32 RandomIdxIdx = FMath::RandRange(0, AvailableMainRoomIndices.Num() - 1);

	// 선택된 실제 RoomList 인덱스를 저장
	PlayerSpawnRoomIndex = AvailableMainRoomIndices[RandomIdxIdx];

	// 저장된 인덱스로 실제 방 데이터를 가져옴
	FDungeonRoom SelectedRoom = RoomList[PlayerSpawnRoomIndex];

	FVector SpawnLocation = SelectedRoom.CenterLocation;
	SpawnLocation.Z += 1000.0f; // 플레이어는 공중에서 떨어지는 게 안전

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (PlayerPawn)
	{
		PlayerPawn->TeleportTo(SpawnLocation, PlayerPawn->GetActorRotation());
	}
}

void ADungeonGenerator::DrawDebugRooms()
{
	FlushPersistentDebugLines(GetWorld());

	for (const FDungeonRoom& Room : RoomList)
	{
		FVector Center = Room.CenterLocation;
		FVector Extent = FVector(Room.Size.X * 0.5f, Room.Size.Y * 0.5f, 50.0f);

		FColor DrawColor = Room.bIsMainRoom ? FColor::Green : FColor::Red;
		float LineThickness = Room.bIsMainRoom ? 20.0f : 5.0f;

		DrawDebugBox(GetWorld(), Center, Extent, DrawColor, true, -1.0f, 0, LineThickness);
	}

	for (const FRoomEdge& Edge : FinalPaths)
	{
		FVector StartPoint = RoomList[Edge.RoomAIndex].CenterLocation;
		FVector EndPoint = RoomList[Edge.RoomBIndex].CenterLocation;

		StartPoint.Z += 100.0f;
		EndPoint.Z += 100.0f;

		DrawDebugLine(GetWorld(), StartPoint, EndPoint, FColor::Blue, true, -1.0f, 0, 15.0f);
	}

	for (const FVector& TilePos : CorridorTiles)
	{
		FVector Extent = FVector(GridSize * 0.5f, GridSize * 0.5f, 10.0f);
		FVector AdjustedPos = TilePos;
		AdjustedPos.Z -= 20.0f;

		DrawDebugSolidBox(GetWorld(), AdjustedPos, Extent, FColor::Yellow, true, -1.0f, 0);
	}
}

void ADungeonGenerator::SpawnEnemies()
{
	if (!EnemyPrefab) return;

	for (int32 i = 0; i < RoomList.Num(); i++)
	{
		const FDungeonRoom& Room = RoomList[i];

		if (!Room.bIsMainRoom || i == PlayerSpawnRoomIndex) continue;

		int32 NumEnemiesToSpawn = FMath::RandRange(MinEnemiesPerRoom, MaxEnemiesPerRoom);

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
				float MinX = Room.CenterLocation.X - (Room.Size.X * 0.5f) + Padding;
				float MaxX = Room.CenterLocation.X + (Room.Size.X * 0.5f) - Padding;
				float MinY = Room.CenterLocation.Y - (Room.Size.Y * 0.5f) + Padding;
				float MaxY = Room.CenterLocation.Y + (Room.Size.Y * 0.5f) - Padding;

				float RandomX = FMath::RandRange(MinX, MaxX);
				float RandomY = FMath::RandRange(MinY, MaxY);

				// 하늘에서 바닥으로 수직 레이저를 쏴서 지형의 실제 높이를 찾기
				FVector TraceStart = FVector(RandomX, RandomY, Room.CenterLocation.Z + 2000.0f); // 방의 아주 높은 곳
				FVector TraceEnd = FVector(RandomX, RandomY, Room.CenterLocation.Z - 500.0f);  // 방의 바닥 아래

				FHitResult HitResult;
				FCollisionQueryParams TraceParams;
				TraceParams.AddIgnoredActor(this);

				// 레이저가 무언가(계단, 바닥, 단상 등)에 부딪혔는지 검사
				if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic, TraceParams))
				{
					// 부딪힌 실제 표면(ImpactPoint)에서, 위로 몬스터의 몸통 절반(약 100.0f)만큼 띄워줌
					SpawnLocation = HitResult.ImpactPoint + FVector(0.0f, 0.0f, 100.0f);

					// 이 정확한 높이의 좌표를 기준으로 공간이 넉넉한지 최종 검사
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
				// 스폰 시 약간의 충돌이 있어도 위치를 조정하며 무조건 스폰되게 하는 옵션
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

				GetWorld()->SpawnActor<AEnemyCharacter>(EnemyPrefab, SpawnLocation, SpawnRotation, SpawnParams);
			}
		}
	}
}

bool ADungeonGenerator::IsSpawnLocationValid(FVector Location)
{
	// 몬스터의 몸통에 맞게 반경을 약간 줄여줍니다 (60 -> 45)
	float CheckRadius = 45.0f;
	FCollisionShape CheckSphere = FCollisionShape::MakeSphere(CheckRadius);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	// Visibility 대신 지형(벽, 계단 등)을 가장 잘 감지하는 WorldStatic 채널 사용
	bool bIsOverlapping = GetWorld()->OverlapAnyTestByChannel(
		Location,
		FQuat::Identity,
		ECC_WorldStatic,
		CheckSphere,
		QueryParams
	);

	if (bIsOverlapping)
	{
		DrawDebugSphere(GetWorld(), Location, CheckRadius, 12, FColor::Red, false, 5.0f);
		return false;
	}

	DrawDebugSphere(GetWorld(), Location, CheckRadius, 12, FColor::Green, false, 5.0f);
	return true;
}

void ADungeonGenerator::RemoveStartingRoomDoors()
{
	// 인덱스가 유효한지 확인
	if (PlayerSpawnRoomIndex < 0 || PlayerSpawnRoomIndex >= RoomList.Num()) return;

	// 시작 방과 연결된 모든 '통로(Edge)' 데이터를 찾아냄
	TArray<FRoomEdge> ConnectedEdges;
	for (const FRoomEdge& Edge : FinalPaths)
	{
		// 시작 방이 A이거나 B인 모든 연결선을 수집
		if (Edge.RoomAIndex == PlayerSpawnRoomIndex || Edge.RoomBIndex == PlayerSpawnRoomIndex)
		{
			ConnectedEdges.Add(Edge);
		}
	}

	// 특정 좌표(문)가 선분(통로) 위에 있는지 확인하는 도우미 람다 함수
	auto IsPointOnSegment = [](FVector P, FVector Start, FVector End, float Tolerance) -> bool
		{
			float MinX = FMath::Min(Start.X, End.X) - Tolerance;
			float MaxX = FMath::Max(Start.X, End.X) + Tolerance;
			float MinY = FMath::Min(Start.Y, End.Y) - Tolerance;
			float MaxY = FMath::Max(Start.Y, End.Y) + Tolerance;

			// 문이 통로 선을 기준으로 Tolerance(오차 범위) 안에 들어오면 true 반환
			return (P.X >= MinX && P.X <= MaxX && P.Y >= MinY && P.Y <= MaxY);
		};

	// 배열의 뒤에서부터 앞으로 거꾸로 검사하며 문을 파괴
	for (int32 i = SpawnedDoors.Num() - 1; i >= 0; --i)
	{
		AActor* Door = SpawnedDoors[i];
		if (!Door) continue;

		FVector DoorLoc = Door->GetActorLocation();
		bool bShouldDestroy = false;

		// 이 문이 '시작 방과 연결된 통로' 중 하나에 겹쳐있는지 검사
		for (const FRoomEdge& Edge : ConnectedEdges)
		{
			FVector PosA = RoomList[Edge.RoomAIndex].CenterLocation;
			FVector PosB = RoomList[Edge.RoomBIndex].CenterLocation;

			float dX = PosB.X - PosA.X;
			float dY = PosB.Y - PosA.Y;

			// 문의 좌표가 완벽하게 중앙이 아닐 수 있으므로 넉넉한 오차 범위를 줌
			float Tol = FMath::Max(CorridorWidth, 300.0f);

			// CreateCorridors에서 사용했던 L자 통로 그리기와 완벽히 동일한 궤적을 검사
			if (FMath::Abs(dX) > FMath::Abs(dY))
			{
				float MidX = (PosA.X + PosB.X) / 2.0f;
				if (IsPointOnSegment(DoorLoc, PosA, FVector(MidX, PosA.Y, 0), Tol) ||
					IsPointOnSegment(DoorLoc, PosB, FVector(MidX, PosB.Y, 0), Tol) ||
					IsPointOnSegment(DoorLoc, FVector(MidX, PosA.Y, 0), FVector(MidX, PosB.Y, 0), Tol))
				{
					bShouldDestroy = true; break;
				}
			}
			else
			{
				float MidY = (PosA.Y + PosB.Y) / 2.0f;
				if (IsPointOnSegment(DoorLoc, PosA, FVector(PosA.X, MidY, 0), Tol) ||
					IsPointOnSegment(DoorLoc, PosB, FVector(PosB.X, MidY, 0), Tol) ||
					IsPointOnSegment(DoorLoc, FVector(PosA.X, MidY, 0), FVector(PosB.X, MidY, 0), Tol))
				{
					bShouldDestroy = true; break;
				}
			}
		}

		// 검사 결과, 통로 위에 있는 문이라면(양쪽 끝 문 모두 포함) 즉시 파괴
		if (bShouldDestroy)
		{
			Door->Destroy();
			SpawnedDoors.RemoveAt(i);
		}
	}
}