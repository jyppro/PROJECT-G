#include "DungeonGenerator.h"
#include "DrawDebugHelpers.h" 
#include "Math/UnrealMathUtility.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/OverlapResult.h"
#include "Kismet/GameplayStatics.h"

ADungeonGenerator::ADungeonGenerator()
{
    // 매 프레임 업데이트가 필요 없으므로 틱(Tick)을 꺼서 성능을 절약합니다.
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

    // 새로 추가! 7단계: 모든 맵 세팅이 끝난 후 플레이어 이동
    TeleportPlayerToRandomRoom();

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

// 1. 방 무작위 생성 (GenerateRandomRooms) 수정
void ADungeonGenerator::GenerateRandomRooms()
{
    RoomList.Empty();

    // 프리팹이 하나도 설정되지 않았다면 안전 종료
    if (MainRoomPrefabs.IsEmpty()) return;

    // 1단계: 에디터에 설정된 모든 방의 가중치(SpawnWeight)를 하나로 다 더합니다.
    float TotalWeight = 0.0f;
    for (const FRoomPrefabData& PrefabData : MainRoomPrefabs)
    {
        TotalWeight += PrefabData.SpawnWeight;
    }

    // 지정한 개수(NumberOfRoomsToGenerate)만큼 100% 방을 만듭니다.
    for (int32 i = 0; i < NumberOfRoomsToGenerate; ++i)
    {
        FDungeonRoom NewRoom;

        // 2단계: 0부터 전체 가중치 합 사이에서 무작위 숫자를 하나 뽑습니다. (룰렛 돌리기!)
        float RandomValue = FMath::FRandRange(0.0f, TotalWeight);
        float AccumulatedWeight = 0.0f;
        int32 SelectedIndex = 0;

        // 3단계: 어떤 방이 당첨되었는지 확인합니다.
        for (int32 j = 0; j < MainRoomPrefabs.Num(); ++j)
        {
            AccumulatedWeight += MainRoomPrefabs[j].SpawnWeight;
            if (RandomValue <= AccumulatedWeight)
            {
                SelectedIndex = j; // 당첨된 방의 인덱스!
                break;
            }
        }

        // 당첨된 방의 인덱스와 실제 크기를 저장합니다.
        NewRoom.SelectedPrefabIndex = SelectedIndex;
        NewRoom.Size = MainRoomPrefabs[SelectedIndex].Size;

        // 위치를 스냅하여 배치
        FVector2D RandomPoint = FMath::RandPointInCircle(SpawnRadius);
        NewRoom.CenterLocation = FVector(SnapToGrid(RandomPoint.X), SnapToGrid(RandomPoint.Y), 0.0f);

        RoomList.Add(NewRoom);
    }
}

bool ADungeonGenerator::DoRoomsOverlap(const FDungeonRoom& A, const FDungeonRoom& B)
{
    // 추가됨: 방의 크기에 RoomPadding의 절반씩 살을 붙여서 '가상의 방어막'을 만듭니다.
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

// 2. 메인 방 선정 (SelectMainRooms) 수정
void ADungeonGenerator::SelectMainRooms()
{
    for (FDungeonRoom& Room : RoomList)
    {
        // 이제 크기 비교를 할 필요 없이, '프리팹이 배정된 방'이면 메인 방이 됩니다.
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

    for (const FRoomEdge& Edge : FinalPaths)
    {
        FVector PosA = RoomList[Edge.RoomAIndex].CenterLocation;
        FVector PosB = RoomList[Edge.RoomBIndex].CenterLocation;

        float dX = PosB.X - PosA.X;
        float dY = PosB.Y - PosA.Y;

        // 마법의 헬퍼 함수(람다): 시작점부터 끝점까지 타일을 깔아주는 기능
        auto DrawLine = [&](FVector Start, FVector End)
            {
                FVector Current = Start;

                // X축으로 먼저 쫙 깔기
                while (!FMath::IsNearlyEqual(Current.X, End.X, 10.0f))
                {
                    float MoveAmount = FMath::Min(CorridorWidth, FMath::Abs(End.X - Current.X));
                    Current.X += FMath::Sign(End.X - Current.X) * MoveAmount;
                    if (!IsPointInAnyMainRoom(Current)) CorridorTiles.AddUnique(Current);
                }
                // 그 다음 Y축으로 쫙 깔기
                while (!FMath::IsNearlyEqual(Current.Y, End.Y, 10.0f))
                {
                    float MoveAmount = FMath::Min(CorridorWidth, FMath::Abs(End.Y - Current.Y));
                    Current.Y += FMath::Sign(End.Y - Current.Y) * MoveAmount;
                    if (!IsPointInAnyMainRoom(Current)) CorridorTiles.AddUnique(Current);
                }
            };

        // 핵심 로직: 무조건 두 방의 '정중앙'에서 나와서 중간에서 만나게(Z자 꺾기) 합니다!
        if (FMath::Abs(dX) > FMath::Abs(dY))
        {
            // 가로로 더 멀리 떨어져 있다면? -> 동/서쪽 문을 사용!
            float MidX = (PosA.X + PosB.X) / 2.0f; // 두 방의 정확히 중간 X 지점

            DrawLine(PosA, FVector(MidX, PosA.Y, 0)); // A방 중앙에서 중간 지점까지 직진!
            DrawLine(PosB, FVector(MidX, PosB.Y, 0)); // B방 중앙에서 중간 지점까지 직진!
            DrawLine(FVector(MidX, PosA.Y, 0), FVector(MidX, PosB.Y, 0)); // 중간 지점에서 세로로 연결!
        }
        else
        {
            // 세로로 더 멀리 떨어져 있다면? -> 남/북쪽 문을 사용!
            float MidY = (PosA.Y + PosB.Y) / 2.0f; // 두 방의 정확히 중간 Y 지점

            DrawLine(PosA, FVector(PosA.X, MidY, 0)); // A방 중앙에서 중간 지점까지 직진!
            DrawLine(PosB, FVector(PosB.X, MidY, 0)); // B방 중앙에서 중간 지점까지 직진!
            DrawLine(FVector(PosA.X, MidY, 0), FVector(PosB.X, MidY, 0)); // 중간 지점에서 가로로 연결!
        }
    }
}

void ADungeonGenerator::SpawnDungeonActors()
{
    // 방 배열이 비어있거나 복도 프리팹이 없다면 안전하게 종료합니다.
    if (MainRoomPrefabs.IsEmpty() || !CorridorPrefab)
    {
        UE_LOG(LogTemp, Warning, TEXT("Prefabs are not assigned! Please check the Details panel."));
        return;
    }

    for (const FDungeonRoom& Room : RoomList)
    {
        // 메인 방이고, 프리팹 인덱스가 정상적으로 있다면 스폰!
        if (Room.bIsMainRoom && Room.SelectedPrefabIndex >= 0)
        {
            FTransform SpawnTransform;
            SpawnTransform.SetLocation(FVector(Room.CenterLocation.X, Room.CenterLocation.Y, 0.0f));

            // 데이터에 이미 저장된 인덱스 번호를 가져와서 해당 프리팹을 스폰합니다.
            TSubclassOf<AActor> SelectedRoomPrefab = MainRoomPrefabs[Room.SelectedPrefabIndex].RoomClass;

            if (SelectedRoomPrefab)
            {
                GetWorld()->SpawnActor<AActor>(SelectedRoomPrefab, SpawnTransform);
                // (스케일 조정 코드는 완전히 삭제되었으므로 원본 크기로 생성됩니다.)
            }
        }
    }

    // 2) 복도 타일 스폰 (이전과 동일)
    for (const FVector& TilePos : CorridorTiles)
    {
        FTransform SpawnTransform;
        SpawnTransform.SetLocation(FVector(TilePos.X, TilePos.Y, 0.0f));

        AActor* SpawnedCorridor = GetWorld()->SpawnActor<AActor>(CorridorPrefab, SpawnTransform);

        if (SpawnedCorridor)
        {
            // GridSize 대신 CorridorWidth로 스케일을 맞춰 얇은 복도로 만듭니다.
            FVector NewScale = FVector(CorridorWidth / 100.0f, CorridorWidth / 100.0f, 1.0f);
            SpawnedCorridor->SetActorScale3D(NewScale);
        }
    }

    // ---------------------------------------------------------
    // 3단계: 복도가 지나가는 길의 '가짜 벽(FakeWall)' 파괴하기
    // ---------------------------------------------------------

    // 복도가 깔린 타일 위치들을 다시 한번 확인합니다.
    for (const FVector& TilePos : CorridorTiles)
    {
        // 1. 검사할 구슬(Sphere) 모양의 영역을 만듭니다. (복도 두께의 절반보다 살짝 크게)
        float CheckRadius = CorridorWidth * 0.6f;
        FCollisionShape CheckSphere = FCollisionShape::MakeSphere(CheckRadius);

        // 2. 바닥이 아니라 '벽'의 높이에서 검사하도록 Z축을 살짝 위로 올려줍니다.
        FVector CheckPos = FVector(TilePos.X, TilePos.Y, 150.0f);

        TArray<FOverlapResult> OverlapResults;
        FCollisionObjectQueryParams ObjectQueryParams = FCollisionObjectQueryParams::AllObjects;

        // 3. 해당 위치에 겹쳐 있는 모든 물체를 찾아냅니다!
        bool bHit = GetWorld()->OverlapMultiByObjectType(
            OverlapResults,
            CheckPos,
            FQuat::Identity,
            ObjectQueryParams,
            CheckSphere
        );

        // 4. 무언가 부딪혔다면?
        if (bHit)
        {
            for (const FOverlapResult& Result : OverlapResults)
            {
                // 부딪힌 물체의 컴포넌트 정보를 가져옵니다.
                UPrimitiveComponent* OverlappedComp = Result.GetComponent();

                // 핵심: 그 컴포넌트가 존재하고, "FakeWall" 이라는 이름표(태그)를 달고 있다면!
                if (OverlappedComp && OverlappedComp->ComponentHasTag(FName("FakeWall")))
                {
                    // 펑! 가짜 벽을 파괴하여 문을 엽니다.
                    OverlappedComp->DestroyComponent();
                }
            }
        }
    }
}

// 새로 추가됨: 복도가 방 내부를 뚫고 지나가지 않게 막아주는 방패 역할!
bool ADungeonGenerator::IsPointInAnyMainRoom(FVector Point)
{
    for (const FDungeonRoom& Room : RoomList)
    {
        if (!Room.bIsMainRoom) continue;

        // 경계선에서 아주 살짝(10) 안쪽으로 판정 범위를 좁힙니다. 
        // (복도 타일이 방 문에 딱 맞닿게 붙도록 하기 위함)
        float Left = Room.CenterLocation.X - (Room.Size.X * 0.5f) + 10.0f;
        float Right = Room.CenterLocation.X + (Room.Size.X * 0.5f) - 10.0f;
        float Top = Room.CenterLocation.Y - (Room.Size.Y * 0.5f) + 10.0f;
        float Bottom = Room.CenterLocation.Y + (Room.Size.Y * 0.5f) - 10.0f;

        // 점(Point)이 방 안쪽에 위치한다면 true 반환
        if (Point.X > Left && Point.X < Right && Point.Y > Top && Point.Y < Bottom)
        {
            return true;
        }
    }
    return false;
}

void ADungeonGenerator::TeleportPlayerToRandomRoom()
{
    // 1. 진짜 방(메인 방)들만 따로 모을 배열을 만듭니다.
    TArray<FDungeonRoom> AvailableRooms;
    for (const FDungeonRoom& Room : RoomList)
    {
        if (Room.bIsMainRoom)
        {
            AvailableRooms.Add(Room);
        }
    }

    // 방이 하나도 없다면 종료
    if (AvailableRooms.IsEmpty()) return;

    // 2. 모아둔 방들 중 무작위로 하나를 뽑습니다.
    int32 RandomIndex = FMath::RandRange(0, AvailableRooms.Num() - 1);
    FDungeonRoom SelectedRoom = AvailableRooms[RandomIndex];

    // 3. 스폰될 위치를 계산합니다. (방의 정중앙)
    FVector SpawnLocation = SelectedRoom.CenterLocation;

    // 꿀팁: 캐릭터가 바닥에 끼지 않도록 Z축(높이)을 살짝 올려서 떨어뜨려 줍니다.
    SpawnLocation.Z += 1000.0f;

    // 4. 현재 월드의 첫 번째 플레이어 캐릭터(인덱스 0)를 찾아냅니다.
    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    if (PlayerPawn)
    {
        // 플레이어의 위치를 뽑힌 방의 위치로 즉시 이동시킵니다!
        PlayerPawn->TeleportTo(SpawnLocation, PlayerPawn->GetActorRotation());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("플레이어 폰을 찾을 수 없습니다!"));
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