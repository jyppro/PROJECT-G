#include "ItemEffect_Painkiller.h"
#include "../Gun_phiriaCharacter.h"
#include "../component/InventoryComponent.h" // 인벤토리 차감용

bool UItemEffect_Painkiller::UseItem_Implementation(AGun_phiriaCharacter* User, FName ItemID) // ItemID 받아오기
{
	if (!User || User->bIsCasting) return false;

	TWeakObjectPtr<AGun_phiriaCharacter> WeakUser(User);

	// [중요] 람다 함수의 캡처 블록 `[ ]` 안에 ItemID를 추가해 줍니다!
	// 이렇게 해야 N초 뒤에 타이머가 터질 때 ItemID가 뭔지 기억하고 지울 수 있습니다.
	User->StartCasting(CastTime, ItemID, [WeakUser, this, ItemID]()
		{
			if (!WeakUser.IsValid()) return;

			// 이제 드디어 에러 없이 인벤토리에서 차감할 수 있습니다!
			if (WeakUser->PlayerInventory)
			{
				WeakUser->PlayerInventory->RemoveItem(ItemID, 1);
			}

			// 캐릭터에 만들어둔 버프 관리 함수 호출
			WeakUser->ApplyHealOverTime(TotalHealAmount, HealDuration);
			WeakUser->ApplySpeedBuff(SpeedBoostAmount, BoostDuration);
		});

	return false;
}