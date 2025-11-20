// Copyright Epic Games, Inc. All Rights Reserved.

#include "LightAwarenessDemoGameMode.h"
#include "LightAwarenessDemoCharacter.h"
#include "UObject/ConstructorHelpers.h"

ALightAwarenessDemoGameMode::ALightAwarenessDemoGameMode()
{
    static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(
        TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.BP_ThirdPersonCharacter_C")
    );

    if (PlayerPawnBPClass.Succeeded())
    {
        DefaultPawnClass = PlayerPawnBPClass.Class;
    }
}
