#include "DungeonGenerator.h"
#include "DrawDebugHelpers.h" 
#include "Math/UnrealMathUtility.h"

ADungeonGenerator::ADungeonGenerator()
{
    // 매 프레임 업데이트가 필요 없으므로 틱(Tick)을 꺼서 성능을 절약합니다.
    PrimaryActorTick.bCanEverTick = false;
}

void ADungeonGenerator::BeginPlay()
{
    Super::BeginPlay();

    GenerateRandomRooms();   // 1단계
    SeparateRooms();         // 2단계
    SelectMainRooms();       // 3단계
    ConnectMainRooms();      // 4단계

    // 5단계: 복도 타일 생성 (새로 추가됨!)
    CreateCorridors();

    if (bShowDebugBoxes)
    {
        DrawDebugRooms();
    }
}

float ADungeonGenerator::SnapToGrid(float Value)
{
    // 그리드 크기의 배수로 반올림합니다.
    return FMath::RoundHalfToEven(Value / GridSize) * GridSize;
}

void ADungeonGenerator::GenerateRandomRooms()
{
    RoomList.Empty();

    for (int32 i = 0; i < NumberOfRoomsToGenerate; ++i)
    {
        FDungeonRoom NewRoom;

        // 크기를 GridSize에 맞춰 스냅(Snap) 합니다.
        float RandomWidth = SnapToGrid(FMath::RandRange(MinRoomSize.X, MaxRoomSize.X));
        float RandomHeight = SnapToGrid(FMath::RandRange(MinRoomSize.Y, MaxRoomSize.Y));

        // 크기가 0이 되는 오류를 방지하기 위해 최소 GridSize로 고정합니다.
        if (RandomWidth < GridSize) RandomWidth = GridSize;
        if (RandomHeight < GridSize) RandomHeight = GridSize;

        NewRoom.Size = FVector2D(RandomWidth, RandomHeight);

        // 위치를 GridSize에 맞춰 스냅(Snap) 합니다.
        FVector2D RandomPoint = FMath::RandPointInCircle(SpawnRadius);
        float SnappedX = SnapToGrid(RandomPoint.X);
        float SnappedY = SnapToGrid(RandomPoint.Y);

        NewRoom.CenterLocation = FVector(SnappedX, SnappedY, 0.0f);

        RoomList.Add(NewRoom);
    }
}

bool ADungeonGenerator::DoRoomsOverlap(const FDungeonRoom& A, const FDungeonRoom& B)
{
    // 각 방의 상하좌우 경계선(좌표)을 계산합니다.
    float LeftA = A.CenterLocation.X - (A.Size.X * 0.5f);
    float RightA = A.CenterLocation.X + (A.Size.X * 0.5f);
    float TopA = A.CenterLocation.Y - (A.Size.Y * 0.5f);
    float BottomA = A.CenterLocation.Y + (A.Size.Y * 0.5f);

    float LeftB = B.CenterLocation.X - (B.Size.X * 0.5f);
    float RightB = B.CenterLocation.X + (B.Size.X * 0.5f);
    float TopB = B.CenterLocation.Y - (B.Size.Y * 0.5f);
    float BottomB = B.CenterLocation.Y + (B.Size.Y * 0.5f);

    // AABB 충돌 공식: 하나라도 안 겹치는 조건이 있다면 충돌하지 않은 것(false)입니다.
    if (RightA <= LeftB || LeftA >= RightB || BottomA <= TopB || TopA >= BottomB)
    {
        return false;
    }

    // 위 조건을 다 통과했다면 두 방은 겹친 것입니다.
    return true;
}

void ADungeonGenerator::SeparateRooms()
{
    bool bHasOverlaps = true;
    int32 MaxIterations = 1000; // 무한 루프 방지용 안전장치
    int32 CurrentIteration = 0;

    // 더 이상 겹치는 방이 없을 때까지(또는 1000번 반복할 때까지) 밀어내기를 반복합니다.
    while (bHasOverlaps && CurrentIteration < MaxIterations)
    {
        bHasOverlaps = false;
        CurrentIteration++;

        // 모든 방의 쌍을 비교합니다.
        for (int i = 0; i < RoomList.Num(); i++)
        {
            for (int j = i + 1; j < RoomList.Num(); j++)
            {
                FDungeonRoom& RoomA = RoomList[i];
                FDungeonRoom& RoomB = RoomList[j];

                if (DoRoomsOverlap(RoomA, RoomB))
                {
                    bHasOverlaps = true; // 겹치는 걸 발견했으므로 다시 루프를 돌아야 합니다.

                    // 밀어낼 방향 계산 (B에서 A로 향하는 방향 벡터)
                    FVector Direction = RoomA.CenterLocation - RoomB.CenterLocation;

                    // 완전히 똑같은 위치에 스폰된 예외 상황 처리
                    if (Direction.IsNearlyZero())
                    {
                        Direction = FVector(FMath::RandRange(-1.f, 1.f), FMath::RandRange(-1.f, 1.f), 0.f);
                    }

                    Direction.Normalize();

                    // 그리드를 유지하기 위해 X축과 Y축 중 더 많이 치우친 방향으로 GridSize 만큼 밀어냅니다.
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

// 1. 함수 구현 추가 (파일 맨 아래나 적당한 곳에 추가하세요)
void ADungeonGenerator::SelectMainRooms()
{
    for (FDungeonRoom& Room : RoomList)
    {
        // 방의 가로, 세로 크기가 모두 설정한 기준치(Threshold) 이상인지 확인합니다.
        if (Room.Size.X >= MainRoomSizeThreshold.X && Room.Size.Y >= MainRoomSizeThreshold.Y)
        {
            Room.bIsMainRoom = true;
        }
        else
        {
            Room.bIsMainRoom = false;
        }
    }
}

// 1. 길을 연결하는 4단계 함수 구현
void ADungeonGenerator::ConnectMainRooms()
{
    FinalPaths.Empty();

    // 1) 모든 메인 방의 인덱스를 수집합니다.
    TArray<int32> MainRoomIndices;
    for (int32 i = 0; i < RoomList.Num(); i++)
    {
        if (RoomList[i].bIsMainRoom)
        {
            MainRoomIndices.Add(i);
        }
    }

    if (MainRoomIndices.Num() == 0) return; // 메인 방이 없으면 종료

    // 2) 프림 알고리즘(Prim's Algorithm)을 위한 리스트 준비
    TArray<int32> ReachedRooms;   // 이미 연결된 방들
    TArray<int32> UnreachedRooms; // 아직 연결되지 않은 방들

    UnreachedRooms = MainRoomIndices;

    // 시작점으로 아무 방이나 하나 고르고 Reached로 옮깁니다.
    ReachedRooms.Add(UnreachedRooms[0]);
    UnreachedRooms.RemoveAt(0);

    // 가능한 모든 추가 경로를 저장해둘 배열 (나중에 루프를 만들기 위함)
    TArray<FRoomEdge> PotentialExtraPaths;

    // 3) 모든 방이 연결될 때까지 반복합니다 (핵심 뼈대 만들기)
    while (UnreachedRooms.Num() > 0)
    {
        float MinDistance = MAX_flt;
        int32 ReachedNodeIndex = -1;
        int32 UnreachedNodeIndex = -1;

        // 연결된 방들과 연결되지 않은 방들 사이의 모든 거리를 재서 가장 짧은 길을 찾습니다.
        for (int32 R_Idx : ReachedRooms)
        {
            for (int32 U_Idx : UnreachedRooms)
            {
                float Distance = FVector::Dist(RoomList[R_Idx].CenterLocation, RoomList[U_Idx].CenterLocation);

                // 추가 경로 후보 저장
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

        // 가장 짧은 길을 최종 경로(FinalPaths)에 추가합니다.
        FRoomEdge NewMSTEdge;
        NewMSTEdge.RoomAIndex = ReachedNodeIndex;
        NewMSTEdge.RoomBIndex = UnreachedNodeIndex;
        NewMSTEdge.Distance = MinDistance;
        FinalPaths.Add(NewMSTEdge);

        // 방금 연결된 방을 Reached로 옮깁니다.
        ReachedRooms.Add(UnreachedNodeIndex);
        UnreachedRooms.Remove(UnreachedNodeIndex);
    }

    // 4) 던전의 재미를 위해 무작위 루프(추가 경로) 만들기
    for (const FRoomEdge& ExtraEdge : PotentialExtraPaths)
    {
        // 이미 MST 핵심 뼈대로 연결된 길은 제외
        if (FinalPaths.Contains(ExtraEdge)) continue;

        // 설정한 확률(예: 15%)에 따라 추가 길을 생성합니다.
        if (FMath::RandRange(0.0f, 1.0f) <= AdditionalPathProbability)
        {
            FinalPaths.Add(ExtraEdge);
        }
    }
}

void ADungeonGenerator::CreateCorridors()
{
    CorridorTiles.Empty();

    // 4단계에서 만든 모든 파란색 선(경로)을 하나씩 꺼내옵니다.
    for (const FRoomEdge& Edge : FinalPaths)
    {
        // 시작 방과 끝 방의 중심 위치
        FVector StartPos = RoomList[Edge.RoomAIndex].CenterLocation;
        FVector EndPos = RoomList[Edge.RoomBIndex].CenterLocation;

        // 현재 복도를 깔고 있는 위치 (시작점에서 출발)
        FVector CurrentPos = StartPos;

        // (1) X축 맞추기: 도착점의 X좌표와 같아질 때까지 이동하며 타일을 깝니다.
        // FMath::IsNearlyEqual을 써서 부동소수점 오차를 안전하게 처리합니다.
        while (!FMath::IsNearlyEqual(CurrentPos.X, EndPos.X, 10.0f))
        {
            // 목표 방향(+인지 -인지)을 구하고, 그리드 크기(GridSize)만큼 한 칸 이동합니다.
            float Direction = FMath::Sign(EndPos.X - CurrentPos.X);
            CurrentPos.X += Direction * GridSize;

            // 이동한 자리에 복도 타일을 추가합니다. (중복 방지)
            CorridorTiles.AddUnique(CurrentPos);
        }

        // (2) Y축 맞추기: X축이 맞춰졌으니, 이제 Y좌표가 같아질 때까지 이동합니다.
        while (!FMath::IsNearlyEqual(CurrentPos.Y, EndPos.Y, 10.0f))
        {
            float Direction = FMath::Sign(EndPos.Y - CurrentPos.Y);
            CurrentPos.Y += Direction * GridSize;

            CorridorTiles.AddUnique(CurrentPos);
        }
    }
}

// 2. 기존의 DrawDebugRooms 함수를 아래와 같이 수정해 주세요.
void ADungeonGenerator::DrawDebugRooms()
{
    FlushPersistentDebugLines(GetWorld());

    for (const FDungeonRoom& Room : RoomList)
    {
        FVector Center = Room.CenterLocation;
        FVector Extent = FVector(Room.Size.X * 0.5f, Room.Size.Y * 0.5f, 50.0f);

        // 메인 방이면 초록색과 굵은 선(20.0f), 일반 방이면 빨간색과 얇은 선(5.0f)으로 설정합니다.
        FColor DrawColor = Room.bIsMainRoom ? FColor::Green : FColor::Red;
        float LineThickness = Room.bIsMainRoom ? 20.0f : 5.0f;

        DrawDebugBox(GetWorld(), Center, Extent, DrawColor, true, -1.0f, 0, LineThickness);
    }

    // 경로(선) 그리기
    for (const FRoomEdge& Edge : FinalPaths)
    {
        FVector StartPoint = RoomList[Edge.RoomAIndex].CenterLocation;
        FVector EndPoint = RoomList[Edge.RoomBIndex].CenterLocation;

        // Z축을 살짝 높여서 선이 박스에 파묻히지 않고 잘 보이게 합니다.
        StartPoint.Z += 100.0f;
        EndPoint.Z += 100.0f;

        // 파란색 선으로 두 방의 중심을 연결합니다.
        DrawDebugLine(GetWorld(), StartPoint, EndPoint, FColor::Blue, true, -1.0f, 0, 15.0f);
    }

    for (const FVector& TilePos : CorridorTiles)
    {
        // 타일 하나는 GridSize 크기의 정사각형입니다. Extent는 절반 크기.
        FVector Extent = FVector(GridSize * 0.5f, GridSize * 0.5f, 25.0f);

        // Z축을 조금 낮춰서 방 박스 아래에 깔린 타일처럼 보이게 합니다.
        FVector AdjustedPos = TilePos;
        AdjustedPos.Z -= 20.0f;

        // 속이 꽉 찬(Solid) 노란색 박스로 시각화합니다.
        DrawDebugSolidBox(GetWorld(), AdjustedPos, Extent, FColor::Yellow, true, -1.0f, 0);
    }
}