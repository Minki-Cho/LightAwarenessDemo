#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WBP_DetectionBar.generated.h"

class AEnemyCharacter;

UCLASS()
class UWBP_DetectionBar : public UUserWidget
{
    GENERATED_BODY()

public:

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Awareness")
    AEnemyCharacter* OwnerEnemy;
};
