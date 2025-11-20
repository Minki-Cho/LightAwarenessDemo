#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LightAwarenessConfig.h"
#include "LightAwarenessDemoCharacter.h"
#include "BehaviorTree/BehaviorTree.h"

#include "EnemyCharacter.generated.h"

UCLASS()
class AEnemyCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AEnemyCharacter();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // ⭐ Behavior Tree asset
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
    TObjectPtr<UBehaviorTree> BehaviorTree;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Awareness")
    TObjectPtr<class UWidgetComponent> DetectionWidgetComp;

    UPROPERTY(BlueprintReadOnly, Category = "Awareness")
    float DetectionLevel = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "Awareness")
    bool bPlayerDetected = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Awareness")
    float AwarenessThreshold = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Awareness")
    TObjectPtr<ULightAwarenessConfig> Config;

    ACharacter* PlayerRef;


    virtual void PossessedBy(AController* NewController) override;

private:
    FTimerHandle SampleTimer;
    void SampleAndIntegrate();
};
