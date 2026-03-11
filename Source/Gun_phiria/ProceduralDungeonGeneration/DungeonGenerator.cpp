#include "DungeonGenerator.h"
#include "DrawDebugHelpers.h" 
#include "Math/UnrealMathUtility.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/OverlapResult.h"
#include "Kismet/GameplayStatics.h"

ADungeonGenerator::ADungeonGenerator()
{
    PrimaryActorTick.bCanEverTick = false;
}

void ADungeonGenerator::BeginPlay()
{
    Super::BeginPlay();

    GenerateRandomRooms();   // 1단계: 생성
    SeparateRooms();         // 2단계: 분리
    SelectMainRooms();       // 3단계: 메인 방 선정
    ConnectMainRooms();      // 4단계: 경로 뼈대 연결
    CreateCorridors();       // 5단계: 복도 타일 계산
    SpawnDungeonActors();    // 6단계: 3D 액터 스폰 및 가짜 벽 파괴

    TeleportPlayerToRandomRoom();

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
        if (Room.SelectedPrefabIndex != -1)
        {
            Room.bIsMainRoom = true;
        }
        else
        {
            Room.bIsMainRoom = false;
        }
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
    TArray<int32> UnreachedRooms;

    UnreachedRooms = MainRoomIndices;

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

    // ---------------------------------------------------------
    // 1) 방 스폰 로직
    // ---------------------------------------------------------
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

    // ---------------------------------------------------------
    // 2) 복도 바닥 스폰 로직
    // ---------------------------------------------------------
    for (const FVector& TilePos : CorridorTiles)
    {
        if (CorridorPrefab)
        {
            FVector SpawnPos = FVector(TilePos.X, TilePos.Y, 0.0f);
            FRotator SpawnRot = FRotator::ZeroRotator;

            AActor* SpawnedCorridor = GetWorld()->SpawnActor<AActor>(CorridorPrefab, SpawnPos, SpawnRot);

            if (SpawnedCorridor)
            {
                FVector NewScale = FVector(CorridorWidth / 100.0f, CorridorWidth / 100.0f, 1.0f);
                SpawnedCorridor->SetActorScale3D(NewScale);
            }
        }
    }

    // ---------------------------------------------------------
    // 2.5) 복도 양옆 벽 스폰 로직
    // ---------------------------------------------------------

    // 헬퍼 람다 함수: 특정 방향에 다른 복도 타일이 이어져 있는지 확인
    auto HasAdjacentCorridor = [&](FVector CheckPos) -> bool
        {
            for (const FVector& Tile : CorridorTiles)
            {
                // 타일 간의 거리가 복도 너비와 비슷하면 길이 이어져 있다고 판단
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

        // 동서남북 4방향 검사 좌표
        FVector NorthPos = TilePos + FVector(0.0f, CorridorWidth, 0.0f);
        FVector SouthPos = TilePos + FVector(0.0f, -CorridorWidth, 0.0f);
        FVector EastPos = TilePos + FVector(CorridorWidth, 0.0f, 0.0f);
        FVector WestPos = TilePos + FVector(-CorridorWidth, 0.0f, 0.0f);

        float WallThickness = 50.0f; // 얇은 벽 두께
        float WallHeight = 500.0f;   // 플레이어를 막아줄 높이

        // 북쪽(+Y)에 길이 없고, 방 내부도 아니면 벽 스폰
        if (!HasAdjacentCorridor(NorthPos) && !IsPointInAnyMainRoom(NorthPos))
        {
            FVector WallPos = TilePos + FVector(0.0f, CorridorWidth * 0.5f, WallHeight * 0.5f);
            AActor* Wall = GetWorld()->SpawnActor<AActor>(CorridorPrefab, WallPos, FRotator::ZeroRotator);
            if (Wall) Wall->SetActorScale3D(FVector(CorridorWidth / 100.0f, WallThickness / 100.0f, WallHeight / 100.0f));
        }
        // 남쪽(-Y)에 길이 없고, 방 내부도 아니면 벽 스폰
        if (!HasAdjacentCorridor(SouthPos) && !IsPointInAnyMainRoom(SouthPos))
        {
            FVector WallPos = TilePos + FVector(0.0f, -CorridorWidth * 0.5f, WallHeight * 0.5f);
            AActor* Wall = GetWorld()->SpawnActor<AActor>(CorridorPrefab, WallPos, FRotator::ZeroRotator);
            if (Wall) Wall->SetActorScale3D(FVector(CorridorWidth / 100.0f, WallThickness / 100.0f, WallHeight / 100.0f));
        }
        // 동쪽(+X)
        if (!HasAdjacentCorridor(EastPos) && !IsPointInAnyMainRoom(EastPos))
        {
            FVector WallPos = TilePos + FVector(CorridorWidth * 0.5f, 0.0f, WallHeight * 0.5f);
            AActor* Wall = GetWorld()->SpawnActor<AActor>(CorridorPrefab, WallPos, FRotator::ZeroRotator);
            if (Wall) Wall->SetActorScale3D(FVector(WallThickness / 100.0f, CorridorWidth / 100.0f, WallHeight / 100.0f));
        }
        // 서쪽(-X)
        if (!HasAdjacentCorridor(WestPos) && !IsPointInAnyMainRoom(WestPos))
        {
            FVector WallPos = TilePos + FVector(-CorridorWidth * 0.5f, 0.0f, WallHeight * 0.5f);
            AActor* Wall = GetWorld()->SpawnActor<AActor>(CorridorPrefab, WallPos, FRotator::ZeroRotator);
            if (Wall) Wall->SetActorScale3D(FVector(WallThickness / 100.0f, CorridorWidth / 100.0f, WallHeight / 100.0f));
        }
    }

    // ---------------------------------------------------------
    // 3) 가짜 벽 파괴 및 문(Door) 생성 로직
    // ---------------------------------------------------------
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
                    // 벽을 부수기 전에, 그 위치와 회전, 크기(Scale)를 모두 복사
                    FTransform DoorTransform = OverlappedComp->GetComponentTransform();

                    if (DoorPrefab)
                    {
                        // 1. 문을 먼저 스폰
                        AActor* SpawnedDoor = GetWorld()->SpawnActor<AActor>(DoorPrefab, DoorTransform);

                        // 2. 스폰된 문에 가짜 벽의 크기(Scale)를 강제로 덮어씌움
                        if (SpawnedDoor)
                        {
                            SpawnedDoor->SetActorScale3D(DoorTransform.GetScale3D());
                        }
                    }

                    // 문을 생성했으니 원래 있던 가짜 큐브 벽은 파괴
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
    TArray<FDungeonRoom> AvailableRooms;
    for (const FDungeonRoom& Room : RoomList)
    {
        if (Room.bIsMainRoom)
        {
            AvailableRooms.Add(Room);
        }
    }

    if (AvailableRooms.IsEmpty()) return;

    int32 RandomIndex = FMath::RandRange(0, AvailableRooms.Num() - 1);
    FDungeonRoom SelectedRoom = AvailableRooms[RandomIndex];

    FVector SpawnLocation = SelectedRoom.CenterLocation;
    SpawnLocation.Z += 1000.0f;

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

    // 예전 배열(TArray) 기반의 깔끔한 디버그 코드로 복구
    for (const FVector& TilePos : CorridorTiles)
    {
        FVector Extent = FVector(GridSize * 0.5f, GridSize * 0.5f, 10.0f);

        FVector AdjustedPos = TilePos;
        AdjustedPos.Z -= 20.0f;

        DrawDebugSolidBox(GetWorld(), AdjustedPos, Extent, FColor::Yellow, true, -1.0f, 0);
    }
}