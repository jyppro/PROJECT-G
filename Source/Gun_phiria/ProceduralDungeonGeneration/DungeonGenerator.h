#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DungeonGenerator.generated.h"

// 구조체 정의 (Structs)

// 개별 방의 데이터를 담을 구조체
USTRUCT(BlueprintType)
struct FRoomPrefabData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AActor> RoomClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D Size = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SpawnWeight = 10.0f;
};

// 생성된 방의 상태를 관리하는 구조체
USTRUCT(BlueprintType)
struct FDungeonRoom
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector CenterLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D Size = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsMainRoom = false;

	UPROPERTY(VisibleAnywhere)
	int32 SelectedPrefabIndex = -1;
};

// 방과 방을 연결하는 '길(Edge)' 구조체
USTRUCT(BlueprintType)
struct FRoomEdge
{
	GENERATED_BODY()

	UPROPERTY()
	int32 RoomAIndex = -1;

	UPROPERTY()
	int32 RoomBIndex = -1;

	UPROPERTY()
	float Distance = 0.0f;

	bool operator==(const FRoomEdge& Other) const
	{
		return (RoomAIndex == Other.RoomAIndex && RoomBIndex == Other.RoomBIndex) ||
			(RoomAIndex == Other.RoomBIndex && RoomBIndex == Other.RoomAIndex);
	}
};

// ==========================================

// 던전 생성기 (Dungeon Generator Actor)
UCLASS()
class GUN_PHIRIA_API ADungeonGenerator : public AActor
{
	GENERATED_BODY()

public:
	ADungeonGenerator();

	// Blueprint에서 수동으로 생성 과정을 제어할 수 있도록 노출된 함수들
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

	// 스폰할 적 클래스 (BP_Enemy를 여기에 넣게 됩니다)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning|Enemy")
	TSubclassOf<class AEnemyCharacter> EnemyPrefab;

	// 한 방에 스폰될 적의 최소 마릿수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning|Enemy")
	int32 MinEnemiesPerRoom = 1;

	// 한 방에 스폰될 적의 최대 마릿수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning|Enemy")
	int32 MaxEnemiesPerRoom = 3;

protected:
	virtual void BeginPlay() override;

	void SetupRoomManagers();

	//// 적들을 방마다 스폰하는 함수
	//void SpawnEnemies();

	//bool IsSpawnLocationValid(FVector Location);

	// =========================================================

	// 던전 설정 (Dungeon Settings)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Settings")
	int32 NumberOfRoomsToGenerate = 50;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Settings")
	float SpawnRadius = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Settings")
	float GridSize = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Settings")
	float RoomPadding = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Settings")
	float CorridorWidth = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Settings")
	float AdditionalPathProbability = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Settings")
	bool bShowDebugBoxes = true;

	// =========================================================

	// 프리팹 설정 (Prefabs)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Spawning")
	TArray<FRoomPrefabData> MainRoomPrefabs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Spawning")
	TSubclassOf<AActor> CorridorPrefab;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Spawning")
	TSubclassOf<AActor> DoorPrefab;

	// ★ [새로 추가] 방 관리자(Room Manager) 프리팹
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Spawning")
	TSubclassOf<class ADungeonRoomManager> RoomManagerPrefab;

	// =========================================================

	// 내부 데이터 (Internal Data)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dungeon Data")
	TArray<FDungeonRoom> RoomList;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dungeon Data")
	TArray<FRoomEdge> FinalPaths;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dungeon Data")
	TArray<FVector> CorridorTiles;

private:
	// 내부 헬퍼 함수들
	float SnapToGrid(float Value);
	bool DoRoomsOverlap(const FDungeonRoom& RoomA, const FDungeonRoom& RoomB);
	bool IsPointInAnyMainRoom(FVector Point);
	void DrawDebugRooms();

	// 플레이어가 스폰된 시작 방의 인덱스를 저장 (기본값 -1)
	int32 PlayerSpawnRoomIndex;

	// 스폰된 문들을 기억해둘 배열
	UPROPERTY()
	TArray<AActor*> SpawnedDoors;

	// 시작 방의 문을 제거하는 함수
	//void RemoveStartingRoomDoors();
};