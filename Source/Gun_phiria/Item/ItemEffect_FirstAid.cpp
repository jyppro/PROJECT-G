#include "ItemEffect_FirstAid.h"
#include "../Gun_phiriaCharacter.h" // 캐릭터 헤더 인클루드

bool UItemEffect_FirstAid::UseItem_Implementation(AGun_phiriaCharacter* User)
{
	if (!User) return false;

	// 이미 체력이 꽉 차 있다면 사용 불가 (아이템 소모 안 함)
	if (User->CurrentHealth >= User->MaxHealth)
	{
		// (필요하다면 여기에 "체력이 가득 찼습니다" UI 메시지 띄우기)
		return false;
	}

	// 최대 체력의 80% 만큼 계산
	float HealAmount = User->MaxHealth * HealPercentage;

	// 체력 회복 적용 (최대 체력을 넘지 않도록 Clamp)
	User->CurrentHealth = FMath::Clamp(User->CurrentHealth + HealAmount, 0.0f, User->MaxHealth);

	// 사용 성공! (이 true를 받아서 인벤토리에서 개수를 1개 깎을 거야)
	return true;
}