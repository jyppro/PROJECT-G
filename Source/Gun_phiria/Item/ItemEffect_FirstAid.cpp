#include "ItemEffect_FirstAid.h"
#include "../Gun_phiriaCharacter.h"
#include "../component/InventoryComponent.h" // 인벤토리 차감용

bool UItemEffect_FirstAid::UseItem_Implementation(AGun_phiriaCharacter* User, FName ItemID)
{
	// 캐릭터가 없거나 이미 시전 중이면 중복 실행 방지
	if (!User || User->bIsCasting) return false;

	// 이미 체력이 꽉 차 있다면 캐스팅조차 시작하지 않음 (아이템 소모 안 함)
	if (User->CurrentHealth >= User->MaxHealth)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("체력이 이미 가득 차서 사용할 수 없습니다."));
		return false;
	}

	// 캐릭터의 안전한 참조 생성 (람다 내부용)
	TWeakObjectPtr<AGun_phiriaCharacter> WeakUser(User);

	// 캐릭터에게 시전을 요청합니다.
	User->StartCasting(CastTime, ItemID, [WeakUser, this, ItemID]()
		{
			// 시전이 성공적으로 끝났을 때 캐릭터가 살아있는지 확인
			if (!WeakUser.IsValid()) return;

			// 1. 인벤토리에서 구급상자/붕대 1개 차감
			if (WeakUser->PlayerInventory)
			{
				WeakUser->PlayerInventory->RemoveItem(ItemID, 1);
			}

			// 2. 최대 체력의 설정된 퍼센트(기본 80%) 만큼 계산
			float HealAmount = WeakUser->MaxHealth * HealPercentage;

			// 3. 체력 회복 적용 (최대 체력을 넘지 않도록 Clamp)
			// 진통제와 달리 HOT(지속회복)가 아니라 캐스팅 직후 즉시 회복!
			WeakUser->CurrentHealth = FMath::Clamp(WeakUser->CurrentHealth + HealAmount, 0.0f, WeakUser->MaxHealth);

		});

	// 시전 시작 시점에서는 즉시 삭제되지 않도록 false 반환
	return false;
}