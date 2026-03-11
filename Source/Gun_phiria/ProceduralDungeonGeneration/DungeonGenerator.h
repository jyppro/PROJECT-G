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

    // 새로 추가! 이 방이 스폰될 확률 가중치 (값이 클수록 자주 나옴)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SpawnWeight;

    FRoomPrefabData()
    {
        // 기본값은 10으로 설정해둡니다.
        SpawnWeight = 10.0f;
    }
};

USTRUCT(BlueprintType)
struct FDungeonRoom
{
    GENERATED_BODY()

    // 방의 중심 위치
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector CenterLocation;

    // 방의 가로, 세로 크기
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector2D Size;

    // 이 방이 메인 방(Main Room)인지 여부 (3단계에서 사용)
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

    // 연결된 두 방의 인덱스 (RoomList 배열의 번호)
    UPROPERTY()
    int32 RoomAIndex;

    UPROPERTY()
    int32 RoomBIndex;

    // 두 방 사이의 거리
    UPROPERTY()
    float Distance;

    FRoomEdge()
    {
        RoomAIndex = -1;
        RoomBIndex = -1;
        Distance = 0.0f;
    }

    // 두 경로가 같은지 비교하기 위한 연산자 오버로딩
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
    // 게임 시작 시 실행되는 함수
    virtual void BeginPlay() override;

public:

    // --- [던전 설정 변수들] ---

    // 생성할 방의 총 개수
    UPROPERTY(EditAnywhere, Category = "Dungeon Settings")
    int32 NumberOfRoomsToGenerate = 50;

    // 방이 최초로 스폰될 원의 반경
    UPROPERTY(EditAnywhere, Category = "Dungeon Settings")
    float SpawnRadius = 2000.0f;

    // 그리드(타일)의 기본 크기 (FPS 기준 1000유닛 = 10m)
    UPROPERTY(EditAnywhere, Category = "Dungeon Settings")
    float GridSize = 1000.0f;

    // 방의 최소 크기 (가로, 세로)
    UPROPERTY(EditAnywhere, Category = "Dungeon Settings")
    FVector2D MinRoomSize = FVector2D(1000.0f, 1000.0f);

    // 방의 최대 크기 (가로, 세로)
    UPROPERTY(EditAnywhere, Category = "Dungeon Settings")
    FVector2D MaxRoomSize = FVector2D(3000.0f, 3000.0f);

    // 생성된 방들을 디버그 선으로 보여줄지 여부
    UPROPERTY(EditAnywhere, Category = "Dungeon Settings")
    bool bShowDebugBoxes = true;

    // 메인 방으로 선정될 최소 크기 조건 (예: 가로 2000, 세로 2000 이상)
    UPROPERTY(EditAnywhere, Category = "Dungeon Settings")
    FVector2D MainRoomSizeThreshold = FVector2D(2000.0f, 2000.0f);

    // 막다른 길을 방지하기 위해 추가로 길을 연결할 확률 (예: 0.15 = 15%)
    UPROPERTY(EditAnywhere, Category = "Dungeon Settings")
    float AdditionalPathProbability = 0.0f;

    // 6단계: 에디터에서 할당할 다수의 메인 방 프리팹들과 각각의 크기
    UPROPERTY(EditAnywhere, Category = "Dungeon Spawning")
    TArray<FRoomPrefabData> MainRoomPrefabs;

    // 6단계: 에디터에서 할당할 복도 프리팹(클래스)
    UPROPERTY(EditAnywhere, Category = "Dungeon Spawning")
    TSubclassOf<AActor> CorridorPrefab;

    // 방과 방 사이의 최소 간격 (예: 1000 = 최소 1타일 이상 떨어짐)
    UPROPERTY(EditAnywhere, Category = "Dungeon Settings")
    float RoomPadding = 1000.0f;

    // 복도의 두께 (예: 500으로 설정하면 기존보다 얇고 세련된 복도가 됩니다)
    UPROPERTY(EditAnywhere, Category = "Dungeon Settings")
    float CorridorWidth = 500.0f;


    // --- [던전 데이터 관리] ---

    // 생성된 모든 방 데이터를 담고 있는 리스트
    UPROPERTY(VisibleAnywhere, Category = "Dungeon Data")
    TArray<FDungeonRoom> RoomList;

    // 생성된 모든 경로(선) 데이터
    UPROPERTY(VisibleAnywhere, Category = "Dungeon Data")
    TArray<FRoomEdge> FinalPaths;

    // 복도를 구성하는 타일들의 중심점 위치를 저장할 배열
    UPROPERTY(VisibleAnywhere, Category = "Dungeon Data")
    TArray<FVector> CorridorTiles;


    // --- [핵심 기능 함수들] ---

    // 값을 그리드 크기에 맞춰 반올림해 주는 헬퍼 함수
    float SnapToGrid(float Value);

    // 1단계: 무작위 크기와 위치의 방을 생성하는 함수
    UFUNCTION(BlueprintCallable, Category = "Dungeon Generation")
    void GenerateRandomRooms();

    // 두 방이 겹치는지 확인하는 헬퍼 함수 (AABB 충돌)
    bool DoRoomsOverlap(const FDungeonRoom& RoomA, const FDungeonRoom& RoomB);

    // 2단계: 겹쳐있는 방들을 흩어지게 밀어내는 함수
    UFUNCTION(BlueprintCallable, Category = "Dungeon Generation")
    void SeparateRooms();

    // 3단계: 크기가 큰 방을 메인 방으로 선정하는 함수
    UFUNCTION(BlueprintCallable, Category = "Dungeon Generation")
    void SelectMainRooms();

    // 4단계: 메인 방들을 연결하는 함수 (프림 알고리즘)
    UFUNCTION(BlueprintCallable, Category = "Dungeon Generation")
    void ConnectMainRooms();

    // 5단계: 메인 방 사이를 잇는 L자형 복도 타일을 생성하는 함수
    UFUNCTION(BlueprintCallable, Category = "Dungeon Generation")
    void CreateCorridors();

    // 6단계: 계산된 데이터를 바탕으로 실제 3D 액터를 월드에 생성하는 함수
    UFUNCTION(BlueprintCallable, Category = "Dungeon Generation")
    void SpawnDungeonActors();

    // 7단계: 플레이어를 무작위 방으로 스폰(이동)시키는 함수
    UFUNCTION(BlueprintCallable, Category = "Dungeon Generation")
    void TeleportPlayerToRandomRoom();

    // 특정 좌표가 '방 내부'에 있는지 확인하는 헬퍼 함수
    bool IsPointInAnyMainRoom(FVector Point);

    // 디버그 박스를 그리는 헬퍼 함수
    void DrawDebugRooms();

};