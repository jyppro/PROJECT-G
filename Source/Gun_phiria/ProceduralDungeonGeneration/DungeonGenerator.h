#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DungeonGenerator.generated.h"

// ==========================================
// 1. 개별 방의 데이터를 담을 구조체 선언
// ==========================================
USTRUCT(BlueprintType)
struct FRoomPrefabData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<AActor> RoomClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector2D Size;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SpawnWeight;

    FRoomPrefabData()
    {
        SpawnWeight = 10.0f;
    }
};

USTRUCT(BlueprintType)
struct FDungeonRoom
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector CenterLocation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector2D Size;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsMainRoom;

    UPROPERTY(VisibleAnywhere)
    int32 SelectedPrefabIndex;

    FDungeonRoom()
    {
        CenterLocation = FVector::ZeroVector;
        Size = FVector2D::ZeroVector;
        bIsMainRoom = false;
        SelectedPrefabIndex = -1;
    }
};

// ==========================================
// 1-2. 방과 방을 연결하는 '길(Edge)' 구조체
// ==========================================
USTRUCT(BlueprintType)
struct FRoomEdge
{
    GENERATED_BODY()

    UPROPERTY()
    int32 RoomAIndex;

    UPROPERTY()
    int32 RoomBIndex;

    UPROPERTY()
    float Distance;

    FRoomEdge()
    {
        RoomAIndex = -1;
        RoomBIndex = -1;
        Distance = 0.0f;
    }

    bool operator==(const FRoomEdge& Other) const
    {
        return (RoomAIndex == Other.RoomAIndex && RoomBIndex == Other.RoomBIndex) ||
            (RoomAIndex == Other.RoomBIndex && RoomBIndex == Other.RoomAIndex);
    }
};

// ==========================================
// 2. 던전 생성기 액터 클래스
// ==========================================
UCLASS()
class GUN_PHIRIA_API ADungeonGenerator : public AActor
{
    GENERATED_BODY()

public:
    ADungeonGenerator();

protected:
    virtual void BeginPlay() override;

public:

    // --- [던전 설정 변수들] ---

    // 생성할 방 갯수
    UPROPERTY(EditAnywhere, Category = "Dungeon Settings")
    int32 NumberOfRoomsToGenerate = 50;

    // 생성 범위
    UPROPERTY(EditAnywhere, Category = "Dungeon Settings")
    float SpawnRadius = 2000.0f;

    // 그리드 크기
    UPROPERTY(EditAnywhere, Category = "Dungeon Settings")
    float GridSize = 1000.0f;

    // 최소 방 사이즈
    UPROPERTY(EditAnywhere, Category = "Dungeon Settings")
    FVector2D MinRoomSize = FVector2D(1000.0f, 1000.0f);

    // 최대 방 사이즈
    UPROPERTY(EditAnywhere, Category = "Dungeon Settings")
    FVector2D MaxRoomSize = FVector2D(3000.0f, 3000.0f);

    // 디버그 설정
    UPROPERTY(EditAnywhere, Category = "Dungeon Settings")
    bool bShowDebugBoxes = true;

    UPROPERTY(EditAnywhere, Category = "Dungeon Settings")
    FVector2D MainRoomSizeThreshold = FVector2D(2000.0f, 2000.0f);

    // 막다른 길 방지를 위한 추가 경로 설정 옵션
    UPROPERTY(EditAnywhere, Category = "Dungeon Settings")
    float AdditionalPathProbability = 0.0f;

    // 방 사이 거리 유지를 위한 패딩
    UPROPERTY(EditAnywhere, Category = "Dungeon Settings")
    float RoomPadding = 1000.0f;

    // 복도 두께
    UPROPERTY(EditAnywhere, Category = "Dungeon Settings")
    float CorridorWidth = 500.0f;

    // 던전 방 프리팹
    UPROPERTY(EditAnywhere, Category = "Dungeon Spawning")
    TArray<FRoomPrefabData> MainRoomPrefabs;

    // 복도 프리팹
    UPROPERTY(EditAnywhere, Category = "Dungeon Spawning")
    TSubclassOf<AActor> CorridorPrefab;

    // 가짜 벽을 대체할 진짜 문 프리팹
    UPROPERTY(EditAnywhere, Category = "Dungeon Spawning")
    TSubclassOf<AActor> DoorPrefab;


    // --- [던전 데이터 관리] ---

    // 방 목록
    UPROPERTY(VisibleAnywhere, Category = "Dungeon Data")
    TArray<FDungeonRoom> RoomList;

    // 최종 복도 경로
    UPROPERTY(VisibleAnywhere, Category = "Dungeon Data")
    TArray<FRoomEdge> FinalPaths;

    // 복도 바닥
    UPROPERTY(VisibleAnywhere, Category = "Dungeon Data")
    TArray<FVector> CorridorTiles;


    // --- [핵심 기능 함수들] ---

    float SnapToGrid(float Value);

    bool DoRoomsOverlap(const FDungeonRoom& RoomA, const FDungeonRoom& RoomB);

    UFUNCTION(BlueprintCallable, Category = "Dungeon Generation")
    void GenerateRandomRooms();

    UFUNCTION(BlueprintCallable, Category = "Dungeon Generation")
    void SeparateRooms();

    UFUNCTION(BlueprintCallable, Category = "Dungeon Generation")
    void SelectMainRooms();

    UFUNCTION(BlueprintCallable, Category = "Dungeon Generation")
    void ConnectMainRooms();

    UFUNCTION(BlueprintCallable, Category = "Dungeon Generation")
    void CreateCorridors();

    UFUNCTION(BlueprintCallable, Category = "Dungeon Generation")
    void SpawnDungeonActors();

    UFUNCTION(BlueprintCallable, Category = "Dungeon Generation")
    void TeleportPlayerToRandomRoom();

    bool IsPointInAnyMainRoom(FVector Point);
    void DrawDebugRooms();
};