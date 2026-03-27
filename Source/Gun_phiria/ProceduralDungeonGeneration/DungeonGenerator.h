#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Kismet/KismetMathLibrary.h" 
#include "DungeonGenerator.generated.h"

// --- Forward Declarations ---
class AEnemyCharacter;
class ADungeonRoomManager;
class APawn;
class UBoxComponent;

// --- Data Structures ---

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

	UPROPERTY()
	bool bIsShopRoom = false;
};

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

UCLASS()
class GUN_PHIRIA_API ADungeonGenerator : public AActor
{
	GENERATED_BODY()

public:
	// --- Constructor ---
	ADungeonGenerator();

	// --- Generation API ---
	UFUNCTION(BlueprintCallable, Category = "Dungeon|Generation")
	void GenerateRandomRooms();

	UFUNCTION(BlueprintCallable, Category = "Dungeon|Generation")
	void SeparateRooms();

	UFUNCTION(BlueprintCallable, Category = "Dungeon|Generation")
	void SelectMainRooms();

	UFUNCTION(BlueprintCallable, Category = "Dungeon|Generation")
	void ConnectMainRooms();

	UFUNCTION(BlueprintCallable, Category = "Dungeon|Generation")
	void CreateCorridors();

	UFUNCTION(BlueprintCallable, Category = "Dungeon|Generation")
	void SpawnDungeonActors();

	UFUNCTION(BlueprintCallable, Category = "Dungeon|Generation")
	void TeleportPlayerToRandomRoom();

protected:
	// --- Lifecycle ---
	virtual void BeginPlay() override;

	// --- Internal Spawning ---
	void SetupRoomManagers();
	void SpawnItemsInRooms();

	// --- Dungeon Settings ---
	UPROPERTY(EditAnywhere, Category = "Dungeon|Settings")
	int32 NumberOfRoomsToGenerate = 50;

	UPROPERTY(EditAnywhere, Category = "Dungeon|Settings")
	float SpawnRadius = 2000.0f;

	UPROPERTY(EditAnywhere, Category = "Dungeon|Settings")
	float GridSize = 1000.0f;

	UPROPERTY(EditAnywhere, Category = "Dungeon|Settings")
	float RoomPadding = 1000.0f;

	UPROPERTY(EditAnywhere, Category = "Dungeon|Settings")
	float CorridorWidth = 500.0f;

	UPROPERTY(EditAnywhere, Category = "Dungeon|Settings")
	float AdditionalPathProbability = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Dungeon|Settings")
	bool bShowDebugBoxes = true;

	// --- Prefab Configuration ---
	UPROPERTY(EditAnywhere, Category = "Dungeon|Prefabs")
	TArray<FRoomPrefabData> MainRoomPrefabs;

	UPROPERTY(EditAnywhere, Category = "Dungeon|Prefabs")
	TSubclassOf<AActor> CorridorPrefab;

	UPROPERTY(EditAnywhere, Category = "Dungeon|Prefabs")
	TSubclassOf<AActor> DoorPrefab;

	UPROPERTY(EditAnywhere, Category = "Dungeon|Prefabs")
	TSubclassOf<ADungeonRoomManager> RoomManagerPrefab;

	// --- Combat & Items ---
	UPROPERTY(EditAnywhere, Category = "Dungeon|Spawning")
	TSubclassOf<AEnemyCharacter> EnemyPrefab;

	UPROPERTY(EditAnywhere, Category = "Dungeon|Spawning")
	int32 MinEnemiesPerRoom = 1;

	UPROPERTY(EditAnywhere, Category = "Dungeon|Spawning")
	int32 MaxEnemiesPerRoom = 3;

	UPROPERTY(EditAnywhere, Category = "Dungeon|Spawning")
	TArray<TSubclassOf<AActor>> ItemPrefabs;

	UPROPERTY(EditAnywhere, Category = "Dungeon|Spawning")
	int32 MinItemsPerRoom = 1;

	UPROPERTY(EditAnywhere, Category = "Dungeon|Spawning")
	int32 MaxItemsPerRoom = 3;

	// --- Dungeon State Data ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dungeon|Data")
	TArray<FDungeonRoom> RoomList;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dungeon|Data")
	TArray<FRoomEdge> FinalPaths;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dungeon|Data")
	TArray<FVector> CorridorTiles;

	UFUNCTION()
	void ExecuteGeneration();

	// 상점 NPC 프리팹
	UPROPERTY(EditAnywhere, Category = "NPC|Prefabs")
	TSubclassOf<class AShopNPC> ShopNPCPrefab;

	// 상점 방을 고르고 NPC를 스폰하는 함수
	void SpawnShopNPC();

	// --- 새로 추가: 상점 가판대(테이블) 프리팹 ---
	UPROPERTY(EditAnywhere, Category = "NPC|Prefabs")
	TSubclassOf<class AActor> ShopStallPrefab;

private:
	// --- Internal Helpers ---
	float SnapToGrid(float Value);
	bool DoRoomsOverlap(const FDungeonRoom& RoomA, const FDungeonRoom& RoomB);
	bool IsPointInAnyMainRoom(FVector Point);
	void DrawDebugRooms();

	// --- State Variables ---
	int32 PlayerSpawnRoomIndex = -1;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> SpawnedDoors;
};