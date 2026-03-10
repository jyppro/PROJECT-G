// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gun_phiriaGameMode.h"
#include "Gun_phiriaCharacter.h"
#include "UObject/ConstructorHelpers.h"

AGun_phiriaGameMode::AGun_phiriaGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
