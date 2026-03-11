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
    UPROPERTY(EditAnywhere, Category = "Dungeon Settings")
    int32 NumberOfRoomsToGenerate = 50;

    UPROPERTY(EditAnywhere, Category = "Dungeon Settings")
    float SpawnRadius = 2000.0f;

    UPROPERTY(EditAnywhere, Category = "Dungeon Settings")
    float GridSize = 1000.0f;

    UPROPERTY(EditAnywhere, Category = "Dungeon Settings")
    FVector2D MinRoomSize = FVector2D(1000.0f, 1000.0f);

    UPROPERTY(EditAnywhere, Category = "Dungeon Settings")
    FVector2D MaxRoomSize = FVector2D(3000.0f, 3000.0f);

    UPROPERTY(EditAnywhere, Category = "Dungeon Settings")
    bool bShowDebugBoxes = true;

    UPROPERTY(EditAnywhere, Category = "Dungeon Settings")
    FVector2D MainRoomSizeThreshold = FVector2D(2000.0f, 2000.0f);

    UPROPERTY(EditAnywhere, Category = "Dungeon Settings")
    float AdditionalPathProbability = 0.0f;

    UPROPERTY(EditAnywhere, Category = "Dungeon Spawning")
    TArray<FRoomPrefabData> MainRoomPrefabs;

    // 복도 에셋 대신, 다시 임시 큐브 스폰용 빈 껍데기 프리팹만 받습니다.
    UPROPERTY(EditAnywhere, Category = "Dungeon Spawning")
    TSubclassOf<AActor> CorridorPrefab;

    UPROPERTY(EditAnywhere, Category = "Dungeon Settings")
    float RoomPadding = 1000.0f;

    // 옛날 코드인 복도 두께(Width)로 완벽 복구!
    UPROPERTY(EditAnywhere, Category = "Dungeon Settings")
    float CorridorWidth = 500.0f;

    // --- [던전 데이터 관리] ---
    UPROPERTY(VisibleAnywhere, Category = "Dungeon Data")
    TArray<FDungeonRoom> RoomList;

    UPROPERTY(VisibleAnywhere, Category = "Dungeon Data")
    TArray<FRoomEdge> FinalPaths;

    // TMap 삭제, 예전 TArray 형태로 완벽 복구!
    UPROPERTY(VisibleAnywhere, Category = "Dungeon Data")
    TArray<FVector> CorridorTiles;

    // --- [핵심 기능 함수들] ---
    float SnapToGrid(float Value);

    UFUNCTION(BlueprintCallable, Category = "Dungeon Generation")
    void GenerateRandomRooms();

    bool DoRoomsOverlap(const FDungeonRoom& RoomA, const FDungeonRoom& RoomB);

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