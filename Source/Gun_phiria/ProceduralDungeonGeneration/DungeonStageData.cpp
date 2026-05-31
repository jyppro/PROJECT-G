#include "DungeonStageData.h"

UDungeonStageData::UDungeonStageData()
{
	// 1. UI 및 노드 표시 정보 기본값 초기화
	StageName = TEXT("Unknown Stage");
	StageDescription = TEXT("새로운 스테이지입니다.");
	StageType = TEXT("Normal");

	// 2. 던전 생성 세팅 기본값 초기화 (기본 방 개수)
	NumberOfRoomsToGenerate = 10;

	// 3. 난이도 세팅 기본값 초기화
	EnemyStatMultiplier = 1.0f;

	// 4. 보상 및 특수 방 필수 스폰 플래그 기본값 초기화 (모두 꺼둠)
	StageItemDataTable = nullptr;
	bForceSpawnShop = false;
	bForceSpawnAnvil = false;
	bForceSpawnGold = false;
	bForceSpawnMiniBoss = false;
	bForceSpawnBoss = false;
}