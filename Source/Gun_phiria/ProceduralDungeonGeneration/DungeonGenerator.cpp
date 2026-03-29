#include "DungeonGenerator.h"
#include "DungeonRoomManager.h"
#include "../Enemy/EnemyCharacter.h"
#include "../Gun_phiriaCharacter.h"
#include "../NPC/ShopNPC.h"
#include "../Item/PickupItemBase.h"

// Engine Headers
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "TimerManager.h"

// Component Headers
#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"

ADungeonGenerator::ADungeonGenerator()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ADungeonGenerator::BeginPlay()
{
	Super::BeginPlay();

	// 1. 맵이 열리자마자 렉이 걸리기 전에 플레이어 화면을 강제로 까맣게 만듭니다.
	if (AGun_phiriaCharacter* PlayerChar = Cast<AGun_phiriaCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)))
	{
		PlayerChar->ForceBlackScreen();
	}

	// 2. 화면이 까맣게 렌더링될 아주 짧은 시간(0.2초)을 벌어준 뒤에 진짜 생성을 시작합니다.
	FTimerHandle GenerationTimerHandle;
	GetWorld()->GetTimerManager().SetTimer(
		GenerationTimerHandle,
		this,
		&ADungeonGenerator::ExecuteGeneration,
		0.2f,
		false
	);
}

void ADungeonGenerator::ExecuteGeneration()
{
	// =========================================================
	// 원래 BeginPlay에 있던 무거운 맵 생성 로직들을 여기서 실행합니다.
	// 화면이 까맣기 때문에 유저는 프레임 드랍을 전혀 느끼지 못합니다.
	// =========================================================
	GenerateRandomRooms();
	SeparateRooms();
	SelectMainRooms();
	ConnectMainRooms();
	CreateCorridors();
	SpawnDungeonActors();

	TeleportPlayerToRandomRoom();

	SpawnShopNPC();

	SetupRoomManagers();
	SpawnItemsInRooms();

	if (bShowDebugBoxes) DrawDebugRooms();

	// 3. 모든 생성이 끝났습니다! 이제 화면을 서서히 밝힙니다.
	if (AGun_phiriaCharacter* PlayerChar = Cast<AGun_phiriaCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)))
	{
		PlayerChar->StartFadeIn(2.0f); // 2초 동안 페이드 인
	}
}

// --- Logic Helpers ---

float ADungeonGenerator::SnapToGrid(float Value)
{
	return FMath::RoundHalfToEven(Value / GridSize) * GridSize;
}

bool ADungeonGenerator::DoRoomsOverlap(const FDungeonRoom& A, const FDungeonRoom& B)
{
	const float Pad = RoomPadding * 0.5f;
	const FBox2D BoxA(
		FVector2D(A.CenterLocation.X - (A.Size.X * 0.5f) - Pad, A.CenterLocation.Y - (A.Size.Y * 0.5f) - Pad),
		FVector2D(A.CenterLocation.X + (A.Size.X * 0.5f) + Pad, A.CenterLocation.Y + (A.Size.Y * 0.5f) + Pad)
	);
	const FBox2D BoxB(
		FVector2D(B.CenterLocation.X - (B.Size.X * 0.5f) - Pad, B.CenterLocation.Y - (B.Size.Y * 0.5f) - Pad),
		FVector2D(B.CenterLocation.X + (B.Size.X * 0.5f) + Pad, B.CenterLocation.Y + (B.Size.Y * 0.5f) + Pad)
	);

	return BoxA.Intersect(BoxB);
}

// --- Generation Logic ---

void ADungeonGenerator::GenerateRandomRooms()
{
	RoomList.Empty();
	if (MainRoomPrefabs.IsEmpty()) return;

	float TotalWeight = 0.0f;
	for (const auto& Prefab : MainRoomPrefabs) TotalWeight += Prefab.SpawnWeight;

	for (int32 i = 0; i < NumberOfRoomsToGenerate; ++i)
	{
		float RandomValue = FMath::FRandRange(0.0f, TotalWeight);
		float AccumulatedWeight = 0.0f;
		int32 SelectedIndex = 0;

		for (int32 j = 0; j < MainRoomPrefabs.Num(); ++j)
		{
			AccumulatedWeight += MainRoomPrefabs[j].SpawnWeight;
			if (RandomValue <= AccumulatedWeight) { SelectedIndex = j; break; }
		}

		FDungeonRoom NewRoom;
		NewRoom.SelectedPrefabIndex = SelectedIndex;
		NewRoom.Size = MainRoomPrefabs[SelectedIndex].Size;

		const FVector2D RandomPoint = FMath::RandPointInCircle(SpawnRadius);
		NewRoom.CenterLocation = FVector(SnapToGrid(RandomPoint.X), SnapToGrid(RandomPoint.Y), 0.0f);
		RoomList.Add(NewRoom);
	}
}

void ADungeonGenerator::SeparateRooms()
{
	bool bHasOverlaps = true;
	int32 Iterations = 0;

	while (bHasOverlaps && Iterations++ < 1000)
	{
		bHasOverlaps = false;
		for (int i = 0; i < RoomList.Num(); i++)
		{
			for (int j = i + 1; j < RoomList.Num(); j++)
			{
				if (DoRoomsOverlap(RoomList[i], RoomList[j]))
				{
					bHasOverlaps = true;
					FVector Dir = RoomList[i].CenterLocation - RoomList[j].CenterLocation;
					if (Dir.IsNearlyZero()) Dir = FVector(FMath::RandRange(-1.f, 1.f), FMath::RandRange(-1.f, 1.f), 0.f);
					Dir.Normalize();

					if (FMath::Abs(Dir.X) > FMath::Abs(Dir.Y))
					{
						RoomList[i].CenterLocation.X += FMath::Sign(Dir.X) * GridSize;
						RoomList[j].CenterLocation.X -= FMath::Sign(Dir.X) * GridSize;
					}
					else
					{
						RoomList[i].CenterLocation.Y += FMath::Sign(Dir.Y) * GridSize;
						RoomList[j].CenterLocation.Y -= FMath::Sign(Dir.Y) * GridSize;
					}
				}
			}
		}
	}
}

void ADungeonGenerator::SelectMainRooms()
{
	for (auto& Room : RoomList) Room.bIsMainRoom = (Room.SelectedPrefabIndex != -1);
}

void ADungeonGenerator::ConnectMainRooms()
{
	FinalPaths.Empty();
	TArray<int32> MainRoomIndices;
	for (int32 i = 0; i < RoomList.Num(); i++) if (RoomList[i].bIsMainRoom) MainRoomIndices.Add(i);

	if (MainRoomIndices.Num() == 0) return;

	TArray<int32> Reached = { MainRoomIndices[0] };
	TArray<int32> Unreached = MainRoomIndices;
	Unreached.RemoveAt(0);

	TArray<FRoomEdge> ExtraPaths;

	while (Unreached.Num() > 0)
	{
		float MinDist = MAX_flt;
		int32 R_Idx = -1, U_Idx = -1;

		for (int32 r : Reached)
		{
			for (int32 u : Unreached)
			{
				float Dist = FVector::Dist(RoomList[r].CenterLocation, RoomList[u].CenterLocation);
				ExtraPaths.AddUnique({ r, u, Dist });
				if (Dist < MinDist) { MinDist = Dist; R_Idx = r; U_Idx = u; }
			}
		}

		FinalPaths.Add({ R_Idx, U_Idx, MinDist });
		Reached.Add(U_Idx);
		Unreached.Remove(U_Idx);
	}

	for (const auto& Edge : ExtraPaths)
	{
		if (!FinalPaths.Contains(Edge) && FMath::FRand() <= AdditionalPathProbability) FinalPaths.Add(Edge);
	}
}

void ADungeonGenerator::CreateCorridors()
{
	CorridorTiles.Empty();

	for (const auto& Edge : FinalPaths)
	{
		FVector A = RoomList[Edge.RoomAIndex].CenterLocation;
		FVector B = RoomList[Edge.RoomBIndex].CenterLocation;

		auto AddTile = [&](FVector P) { if (!IsPointInAnyMainRoom(P)) CorridorTiles.AddUnique(P); };

		auto DrawLine = [&](FVector Start, FVector End)
			{
				FVector Curr = Start;
				while (!FMath::IsNearlyEqual(Curr.X, End.X, 10.f))
				{
					Curr.X += FMath::Sign(End.X - Curr.X) * FMath::Min(CorridorWidth, FMath::Abs(End.X - Curr.X));
					AddTile(Curr);
				}
				while (!FMath::IsNearlyEqual(Curr.Y, End.Y, 10.f))
				{
					Curr.Y += FMath::Sign(End.Y - Curr.Y) * FMath::Min(CorridorWidth, FMath::Abs(End.Y - Curr.Y));
					AddTile(Curr);
				}
			};

		if (FMath::Abs(B.X - A.X) > FMath::Abs(B.Y - A.Y))
		{
			float MidX = (A.X + B.X) * 0.5f;
			DrawLine(A, FVector(MidX, A.Y, 0));
			DrawLine(B, FVector(MidX, B.Y, 0));
			DrawLine(FVector(MidX, A.Y, 0), FVector(MidX, B.Y, 0));
		}
		else
		{
			float MidY = (A.Y + B.Y) * 0.5f;
			DrawLine(A, FVector(A.X, MidY, 0));
			DrawLine(B, FVector(B.X, MidY, 0));
			DrawLine(FVector(A.X, MidY, 0), FVector(B.X, MidY, 0));
		}
	}
}

void ADungeonGenerator::SpawnDungeonActors()
{
	if (MainRoomPrefabs.IsEmpty()) return;

	// 방 스폰
	for (const auto& Room : RoomList)
	{
		if (Room.bIsMainRoom && Room.SelectedPrefabIndex >= 0)
		{
			GetWorld()->SpawnActor<AActor>(MainRoomPrefabs[Room.SelectedPrefabIndex].RoomClass, Room.CenterLocation, FRotator::ZeroRotator);
		}
	}

	// 복도 바닥 스폰
	for (const FVector& Tile : CorridorTiles)
	{
		if (TObjectPtr<AActor> Corridor = GetWorld()->SpawnActor<AActor>(CorridorPrefab, Tile, FRotator::ZeroRotator))
		{
			Corridor->SetActorScale3D(FVector(CorridorWidth / 100.f, CorridorWidth / 100.f, 1.f));
		}
	}

	// 복도 벽 스폰
	auto HasAdjacentCorridor = [&](FVector CheckPos) -> bool
		{
			for (const FVector& Tile : CorridorTiles)
			{
				if (FVector::Dist(Tile, CheckPos) < CorridorWidth * 0.9f) return true;
			}
			return false;
		};

	for (const FVector& TilePos : CorridorTiles)
	{
		if (!CorridorPrefab) continue;

		// 4방향 위치 계산
		TArray<FVector> Directions = {
			TilePos + FVector(0.0f, CorridorWidth, 0.0f),  // North
			TilePos + FVector(0.0f, -CorridorWidth, 0.0f), // South
			TilePos + FVector(CorridorWidth, 0.0f, 0.0f),  // East
			TilePos + FVector(-CorridorWidth, 0.0f, 0.0f)  // West
		};

		const float WallHeight = 500.0f;
		const float WallThickness = 50.0f;

		for (int32 i = 0; i < Directions.Num(); ++i)
		{
			FVector CheckPos = Directions[i];
			if (!HasAdjacentCorridor(CheckPos) && !IsPointInAnyMainRoom(CheckPos))
			{
				FVector Offset = (Directions[i] - TilePos) * 0.5f;
				FVector WallPos = TilePos + Offset + FVector(0.0f, 0.0f, WallHeight * 0.5f);

				if (TObjectPtr<AActor> Wall = GetWorld()->SpawnActor<AActor>(CorridorPrefab, WallPos, FRotator::ZeroRotator))
				{
					// 방향에 따른 스케일 조절 (i < 2는 남북 방향, i >= 2는 동서 방향)
					FVector WallScale = (i < 2) ?
						FVector(CorridorWidth / 100.f, WallThickness / 100.f, WallHeight / 100.f) :
						FVector(WallThickness / 100.f, CorridorWidth / 100.f, WallHeight / 100.f);

					Wall->SetActorScale3D(WallScale);
				}
			}
		}
	}

	// 문 생성
	FCollisionObjectQueryParams ObjectParams = FCollisionObjectQueryParams::AllObjects;
	for (const FVector& Tile : CorridorTiles)
	{
		TArray<FOverlapResult> Overlaps;
		if (GetWorld()->OverlapMultiByObjectType(Overlaps, Tile + FVector(0, 0, 150.f), FQuat::Identity, ObjectParams, FCollisionShape::MakeSphere(CorridorWidth * 0.6f)))
		{
			for (const auto& Result : Overlaps)
			{
				if (Result.GetComponent() && Result.GetComponent()->ComponentHasTag("FakeWall"))
				{
					FTransform DoorTr = Result.GetComponent()->GetComponentTransform();
					if (TObjectPtr<AActor> Door = GetWorld()->SpawnActor<AActor>(DoorPrefab, DoorTr))
					{
						// 문의 크기를 원래 FakeWall의 크기로 설정합니다.
						Door->SetActorScale3D(DoorTr.GetScale3D());

						Door->SetActorHiddenInGame(true);
						Door->SetActorEnableCollision(false);
						SpawnedDoors.Add(Door);
					}
					Result.GetComponent()->DestroyComponent();
				}
			}
		}
	}
}

void ADungeonGenerator::SetupRoomManagers()
{
	if (!RoomManagerPrefab) return;

	for (int32 i = 0; i < RoomList.Num(); i++)
	{
		const auto& Room = RoomList[i];
		if (!Room.bIsMainRoom || i == PlayerSpawnRoomIndex || Room.bIsShopRoom) continue;

		if (TObjectPtr<ADungeonRoomManager> Manager = GetWorld()->SpawnActor<ADungeonRoomManager>(RoomManagerPrefab, Room.CenterLocation + FVector(0, 0, 200.f), FRotator::ZeroRotator))
		{
			Manager->RoomSize = FVector(Room.Size.X, Room.Size.Y, 1000.f);
			Manager->EnemyPrefab = EnemyPrefab;

			if (Manager->RoomTrigger)
				Manager->RoomTrigger->SetBoxExtent(FVector((Room.Size.X * 0.5f) - 150.f, (Room.Size.Y * 0.5f) - 150.f, 500.f));

			for (TObjectPtr<AActor> Door : SpawnedDoors)
			{
				if (!IsValid(Door)) continue;
				FVector Loc = Door->GetActorLocation();
				if (Loc.X >= Room.CenterLocation.X - (Room.Size.X * 0.5f) - CorridorWidth && Loc.X <= Room.CenterLocation.X + (Room.Size.X * 0.5f) + CorridorWidth &&
					Loc.Y >= Room.CenterLocation.Y - (Room.Size.Y * 0.5f) - CorridorWidth && Loc.Y <= Room.CenterLocation.Y + (Room.Size.Y * 0.5f) + CorridorWidth)
				{
					Manager->ConnectedDoors.Add(Door);
				}
			}
		}
	}
}

void ADungeonGenerator::SpawnItemsInRooms()
{
	// 아이템 프리팹이나 전리품 목록이 비어있으면 실행 안 함
	if (ItemPrefabs.IsEmpty() || LootPool.IsEmpty()) return;

	for (const auto& Room : RoomList)
	{
		if (!Room.bIsMainRoom || Room.bIsShopRoom) continue;

		int32 Count = FMath::RandRange(MinItemsPerRoom, MaxItemsPerRoom);
		for (int32 j = 0; j < Count; j++)
		{
			FVector SpawnLoc;
			bool bFound = false;

			for (int32 Tries = 0; Tries < 10 && !bFound; Tries++)
			{
				float RX = FMath::RandRange(Room.CenterLocation.X - Room.Size.X * 0.4f, Room.CenterLocation.X + Room.Size.X * 0.4f);
				float RY = FMath::RandRange(Room.CenterLocation.Y - Room.Size.Y * 0.4f, Room.CenterLocation.Y + Room.Size.Y * 0.4f);

				FHitResult Hit;
				if (GetWorld()->LineTraceSingleByChannel(Hit, FVector(RX, RY, 2000), FVector(RX, RY, -500), ECC_WorldStatic))
				{
					FVector GroundLocation = Hit.ImpactPoint;
					float CheckRadius = 20.0f;

					// 픽스 1: 구슬이 바닥에 쓸려서 '겹침' 판정 나는 것을 막기 위해 반지름보다 5.f 더 위로 띄움!
					FVector CheckLocation = GroundLocation + FVector(0.0f, 0.0f, CheckRadius + 5.0f);

					FCollisionQueryParams OverlapParams;
					OverlapParams.AddIgnoredActor(this);

					if (!GetWorld()->OverlapAnyTestByChannel(CheckLocation, FQuat::Identity, ECC_WorldStatic, FCollisionShape::MakeSphere(CheckRadius), OverlapParams))
					{
						bFound = true;
						SpawnLoc = GroundLocation; // 실제 스폰은 바닥에!
					}
				}
			}

			if (bFound)
			{
				// 픽스 2: 엔진이 바닥과 겹친다고 착각해서 스폰을 취소하는 것을 강제로 막는 파라미터!
				FActorSpawnParameters SpawnParams;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

				// 스폰 파라미터(SpawnParams)를 넣어서 강제 스폰
				AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(
					ItemPrefabs[FMath::RandRange(0, ItemPrefabs.Num() - 1)],
					SpawnLoc,
					FRotator(0, FMath::RandRange(0.f, 360.f), 0),
					SpawnParams
				);

				if (APickupItemBase* PickupItem = Cast<APickupItemBase>(SpawnedActor))
				{
					FName RandomLootID = LootPool[FMath::RandRange(0, LootPool.Num() - 1)];
					PickupItem->ItemID = RandomLootID;

					FString ItemString = RandomLootID.ToString();
					if (ItemString.Contains(TEXT("Ammo"))) PickupItem->Quantity = FMath::RandRange(15, 30);
					else PickupItem->Quantity = 1;
				}
			}
		}
	}
}

// --- Helper Implementation ---

bool ADungeonGenerator::IsPointInAnyMainRoom(FVector Point)
{
	for (const auto& Room : RoomList)
	{
		if (!Room.bIsMainRoom) continue;
		if (FMath::Abs(Point.X - Room.CenterLocation.X) < (Room.Size.X * 0.5f - 10.f) &&
			FMath::Abs(Point.Y - Room.CenterLocation.Y) < (Room.Size.Y * 0.5f - 10.f)) return true;
	}
	return false;
}

void ADungeonGenerator::TeleportPlayerToRandomRoom()
{
	TArray<int32> Mains;
	for (int32 i = 0; i < RoomList.Num(); i++) if (RoomList[i].bIsMainRoom) Mains.Add(i);
	if (Mains.IsEmpty()) return;

	PlayerSpawnRoomIndex = Mains[FMath::RandRange(0, Mains.Num() - 1)];
	if (APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
		Player->TeleportTo(RoomList[PlayerSpawnRoomIndex].CenterLocation + FVector(0, 0, 1000.f), Player->GetActorRotation());
}

void ADungeonGenerator::DrawDebugRooms()
{
#if !UE_BUILD_SHIPPING
	FlushPersistentDebugLines(GetWorld());
	for (const auto& Room : RoomList)
		DrawDebugBox(GetWorld(), Room.CenterLocation, FVector(Room.Size.X * 0.5f, Room.Size.Y * 0.5f, 50), Room.bIsMainRoom ? FColor::Green : FColor::Red, true, -1, 0, Room.bIsMainRoom ? 20 : 5);
#endif
}

void ADungeonGenerator::SpawnShopNPC()
{
	if (!ShopNPCPrefab) return;

	TArray<int32> CandidateIndices;
	// 플레이어 시작 방을 제외한 메인 방을 후보로 등록
	for (int32 i = 0; i < RoomList.Num(); i++)
	{
		if (RoomList[i].bIsMainRoom && i != PlayerSpawnRoomIndex)
		{
			CandidateIndices.Add(i);
		}
	}

	if (CandidateIndices.IsEmpty()) return;

	// 랜덤으로 상점 방 선정
	int32 ShopRoomIndex = CandidateIndices[FMath::RandRange(0, CandidateIndices.Num() - 1)];
	RoomList[ShopRoomIndex].bIsShopRoom = true;

	FVector RoomCenter = RoomList[ShopRoomIndex].CenterLocation;

	// =========================================================
	// 1. NPC가 바라볼 '입구 방향' (회전) 계산 (기존 유지)
	// =========================================================
	TArray<FVector> EntryDirections;

	// FinalPaths에서 상점 방과 연결된 모든 복도의 방향을 찾습니다.
	for (const auto& Path : FinalPaths)
	{
		if (Path.RoomAIndex == ShopRoomIndex)
		{
			FVector ToOtherRoom = RoomList[Path.RoomBIndex].CenterLocation - RoomCenter;
			EntryDirections.Add(ToOtherRoom.GetSafeNormal());
		}
		else if (Path.RoomBIndex == ShopRoomIndex)
		{
			FVector ToOtherRoom = RoomList[Path.RoomAIndex].CenterLocation - RoomCenter;
			EntryDirections.Add(ToOtherRoom.GetSafeNormal());
		}
	}

	// 기본값 (연결된 복도가 없을 때 대비)
	FVector TargetDirVector = FVector::ForwardVector;
	FRotator NPCRotation = FRotator::ZeroRotator;

	// 연결된 복도가 있다면, 그중 랜덤하게 하나를 선택합니다.
	if (!EntryDirections.IsEmpty())
	{
		TargetDirVector = EntryDirections[FMath::RandRange(0, EntryDirections.Num() - 1)];

		// FindLookAtRotation을 사용하여 방향 벡터를 FRotator로 변환합니다.
		NPCRotation = UKismetMathLibrary::FindLookAtRotation(FVector::ZeroVector, TargetDirVector);
		// 상점 주인은 똑바로 서있어야 하므로, Pitch와 Roll을 0으로 고정합니다.
		NPCRotation.Pitch = 0.0f;
		NPCRotation.Roll = 0.0f;
	}

	// =========================================================
	// 2. 바닥 높이 찾기 (LineTrace) (기존 유지)
	// =========================================================
	FHitResult HitResult;
	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(this);

	FVector TraceStart = FVector(RoomCenter.X, RoomCenter.Y, RoomCenter.Z + 2000.0f);
	FVector TraceEnd = FVector(RoomCenter.X, RoomCenter.Y, RoomCenter.Z - 500.0f);

	float GroundZ = RoomCenter.Z;
	if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic, TraceParams))
	{
		GroundZ = HitResult.ImpactPoint.Z;
	}

	// =========================================================
	// 3. NPC 및 가판대 스폰 위치 계산 (정중앙 맞춤, ★완전 수정됨)
	// =========================================================
	// NPC와 가판대 사이의 총 간격을 200.0f로 설정합니다. (네 메시 크기에 맞게 조절하세요!)
	const float TotalOffset = 55.0f;
	const float HalfOffset = TotalOffset * 0.5f;

	// ★가판대 스폰 위치: 방 중앙에서 '입구 방향'으로 HalfOffset만큼 이동 (입구 쪽)
	FVector StallSpawnLoc = RoomCenter + (TargetDirVector * HalfOffset);
	StallSpawnLoc.Z = GroundZ; // 가판대는 바닥에 붙입니다.

	// ★NPC 스폰 위치: 방 중앙에서 '입구 반대 방향'으로 HalfOffset만큼 이동 (가판대 뒤쪽)
	// (TargetDirVector에 마이너스(-)를 곱해서 반대 방향으로 이동합니다)
	FVector NPCSpawnLoc = RoomCenter - (TargetDirVector * HalfOffset);
	// 바닥에서 발 띄우기 (캡슐 Half-Height 96.0f)
	NPCSpawnLoc.Z = GroundZ + 96.0f;

	// =========================================================
	// 4. 스폰 실행
	// =========================================================
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// NPC 스폰 (계산한 NPCRotation 사용, 입구를 바라봅니다)
	AShopNPC* SpawnedNPC = GetWorld()->SpawnActor<AShopNPC>(ShopNPCPrefab, NPCSpawnLoc, NPCRotation, SpawnParams);

	// 가판대 스폰 (NPC와 동일한 방향을 바라보도록 NPCRotation 사용)
	if (SpawnedNPC && ShopStallPrefab)
	{
		GetWorld()->SpawnActor<AActor>(ShopStallPrefab, StallSpawnLoc, NPCRotation, SpawnParams);
	}
}