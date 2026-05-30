#include "DungeonStageData.h"

UDungeonStageData::UDungeonStageData()
{
	// 1. UI 및 노드 표시 정보 기본값 초기화
	StageName = TEXT("Unknown Stage");
	StageDescription = TEXT("새로운 스테이지입니다.");
	StageType = TEXT("Normal");

	// 2. 던전 생성 세팅 기본값 초기화
	NumberOfRoomsToGenerate = 10;

	// Prefab이나 Blueprint 클래스들은 에디터에서 할당하는 것이 안전하므로 
	// C++에서 굳이 nullptr로 명시하지 않아도 자동으로 초기화됨

	// 3. 난이도 세팅 기본값 초기화
	EnemyStatMultiplier = 1.0f;

	// 4. 보상 및 특수 방 세팅 기본값 초기화
	bForceSpawnShop = false;
	StageItemDataTable = nullptr;
}