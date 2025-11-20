#pragma once

#include "Engine/DataAsset.h"
#include "LightAwarenessConfig.generated.h"

UCLASS(BlueprintType)
class LIGHTAWARENESSDEMO_API ULightAwarenessConfig : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float DetectRange = 1500.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float GainScale = 50.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float DecayPerSec = 20.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float MaxPointWeight = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float DistFalloffK = 10.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float OccludedFactor = 0.15f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float SpotSmooth = 0.2f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float LightNorm = 500.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float SampleInterval = 0.05f;
};
