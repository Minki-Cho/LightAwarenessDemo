#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyAIController.generated.h"

class UBlackboardComponent;
class UBehaviorTreeComponent;
class ACharacter;


UCLASS()
class AEnemyAIController : public AAIController
{
    GENERATED_BODY()

public:
    AEnemyAIController();

protected:
    virtual void OnPossess(APawn* InPawn) override;

public:

    UPROPERTY()
    ACharacter* PlayerRef;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<UBlackboardComponent> BlackboardComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<UBehaviorTreeComponent> BehaviorComp;

    void SetPlayerRef(ACharacter* Player);
};
