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
#include "DungeonRoomManager.h"
#include "Components/BoxComponent.h"

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

	SetupRoomManagers();

	SpawnItemsInRooms();

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

							// ★ [새로 추가된 부분] 던전의 모든 문을 기본적으로 '열림' 상태로 만듭니다!
							SpawnedDoor->SetActorHiddenInGame(true);
							SpawnedDoor->SetActorEnableCollision(false);

							// 나중에 찾을 수 있도록 배열에 기억
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

// ★ [새로 추가된 함수] 각 방 중앙에 관리자를 스폰하고 문을 연결해 줍니다.
void ADungeonGenerator::SetupRoomManagers()
{
	if (!RoomManagerPrefab) return;

	for (int32 i = 0; i < RoomList.Num(); i++)
	{
		const FDungeonRoom& Room = RoomList[i];

		// 시작 방이거나 메인 방이 아니면 스킵합니다.
		if (!Room.bIsMainRoom || i == PlayerSpawnRoomIndex) continue;

		// 방 바닥에서 살짝 띄워 트리거 박스 중앙을 맞춥니다.
		FVector SpawnLocation = Room.CenterLocation;
		SpawnLocation.Z += 200.0f;

		// 1. 방 관리자 스폰
		ADungeonRoomManager* RoomManager = GetWorld()->SpawnActor<ADungeonRoomManager>(RoomManagerPrefab, SpawnLocation, FRotator::ZeroRotator);

		if (RoomManager)
		{
			// 2. 방 크기와 에너미 프리팹 전달
			RoomManager->RoomSize = FVector(Room.Size.X, Room.Size.Y, 1000.0f);
			RoomManager->EnemyPrefab = EnemyPrefab; // Generator에 할당된 적 프리팹을 넘겨줌

			// 3. 플레이어 감지용 트리거 박스 크기를 '방 크기에 딱 맞게' 자동 조절!
			// (복도를 지나가다 실수로 발동되지 않도록 방 크기보다 살짝(150) 작게 설정)
			if (UBoxComponent* Trigger = RoomManager->RoomTrigger)
			{
				Trigger->SetBoxExtent(FVector((Room.Size.X * 0.5f) - 150.0f, (Room.Size.Y * 0.5f) - 150.0f, 500.0f));
			}

			// 4. 이 방의 경계선(둘레)에 위치한 문(Door)들을 찾아 관리자에게 귀속시킵니다.
			float Tolerance = CorridorWidth; // 오차 범위
			float MinX = Room.CenterLocation.X - (Room.Size.X * 0.5f) - Tolerance;
			float MaxX = Room.CenterLocation.X + (Room.Size.X * 0.5f) + Tolerance;
			float MinY = Room.CenterLocation.Y - (Room.Size.Y * 0.5f) - Tolerance;
			float MaxY = Room.CenterLocation.Y + (Room.Size.Y * 0.5f) + Tolerance;

			for (AActor* Door : SpawnedDoors)
			{
				// RemoveStartingRoomDoors에서 지워진 문은 무시(IsValid 검사)
				if (!IsValid(Door)) continue;

				FVector DoorLoc = Door->GetActorLocation();

				// 문의 위치가 이 방의 둘레 안에 들어온다면?
				if (DoorLoc.X >= MinX && DoorLoc.X <= MaxX && DoorLoc.Y >= MinY && DoorLoc.Y <= MaxY)
				{
					RoomManager->ConnectedDoors.Add(Door); // 이 방의 문으로 등록!

					// 게임 시작 시점에서는 당연히 모든 문이 열려있어야 합니다.
					Door->SetActorHiddenInGame(true);
					Door->SetActorEnableCollision(false);
				}
			}
		}
	}
}

void ADungeonGenerator::SpawnItemsInRooms()
{
	if (ItemPrefabs.IsEmpty()) return;

	for (int32 i = 0; i < RoomList.Num(); i++)
	{
		const FDungeonRoom& Room = RoomList[i];
		if (!Room.bIsMainRoom) continue;

		int32 NumItemsToSpawn = FMath::RandRange(MinItemsPerRoom, MaxItemsPerRoom);

		for (int32 j = 0; j < NumItemsToSpawn; j++)
		{
			TSubclassOf<AActor> SelectedItemPrefab = ItemPrefabs[FMath::RandRange(0, ItemPrefabs.Num() - 1)];
			if (!SelectedItemPrefab) continue;

			bool bValidPointFound = false;
			FVector SpawnLocation;
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

				FVector TraceStart = FVector(RandomX, RandomY, Room.CenterLocation.Z + 2000.0f);
				FVector TraceEnd = FVector(RandomX, RandomY, Room.CenterLocation.Z - 500.0f);

				FHitResult HitResult;
				FCollisionQueryParams TraceParams;
				TraceParams.AddIgnoredActor(this);

				if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic, TraceParams))
				{
					// ★ 수정: 구체가 바닥에 닿지 않도록 충분히 띄워줍니다 (50.0f)
					SpawnLocation = HitResult.ImpactPoint + FVector(0.0f, 0.0f, 50.0f);

					// ★ 수정: 구체 크기를 조금 줄여줍니다 (20.0f)
					float CheckRadius = 20.0f;
					FCollisionShape CheckSphere = FCollisionShape::MakeSphere(CheckRadius);

					// 겹침 검사
					if (!GetWorld()->OverlapAnyTestByChannel(SpawnLocation, FQuat::Identity, ECC_WorldStatic, CheckSphere, TraceParams))
					{
						bValidPointFound = true;
					}
					else
					{
						// (디버그) 겹쳐서 실패한 위치는 빨간 구체로 표시
						if (bShowDebugBoxes) DrawDebugSphere(GetWorld(), SpawnLocation, CheckRadius, 12, FColor::Red, true, -1.0f);
					}
				}
				else
				{
					// (디버그) 바닥을 아예 못 찾은 레이저는 노란색 선으로 표시
					if (bShowDebugBoxes) DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Yellow, true, -1.0f, 0, 5.0f);
				}
			}

			if (bValidPointFound)
			{
				FRotator SpawnRotation = FRotator(0.0f, FMath::RandRange(0.0f, 360.0f), 0.0f);
				FActorSpawnParameters SpawnParams;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

				AActor* SpawnedItem = GetWorld()->SpawnActor<AActor>(SelectedItemPrefab, SpawnLocation, SpawnRotation, SpawnParams);

				// (디버그) 성공적으로 스폰된 위치는 초록색 선으로 표시!
				if (SpawnedItem && bShowDebugBoxes)
				{
					DrawDebugLine(GetWorld(), SpawnLocation, SpawnLocation + FVector(0, 0, 200), FColor::Green, true, -1.0f, 0, 5.0f);
				}
			}
		}
	}
}