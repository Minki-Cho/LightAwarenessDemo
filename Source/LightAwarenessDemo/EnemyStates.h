#pragma once

#include "EnemyStates.generated.h"

UENUM(BlueprintType)
enum class EAIState : uint8
{
    Idle        UMETA(DisplayName = "Idle"),
    Chase       UMETA(DisplayName = "Chase"),
    Investigate UMETA(DisplayName = "Investigate")
};
