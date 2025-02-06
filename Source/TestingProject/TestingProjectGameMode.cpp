// Copyright Epic Games, Inc. All Rights Reserved.

#include "TestingProjectGameMode.h"
#include "TestingProjectCharacter.h"
#include "UObject/ConstructorHelpers.h"

ATestingProjectGameMode::ATestingProjectGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
