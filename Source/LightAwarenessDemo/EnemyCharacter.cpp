#include "EnemyCharacter.h"
#include "LightAwarenessDemoCharacter.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "WBP_DetectionBar.h"
#include "EnemyAIController.h"
#include "BehaviorTree/BlackboardComponent.h"

AEnemyCharacter::AEnemyCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    DetectionWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("DetectionWidget"));
    DetectionWidgetComp->SetupAttachment(GetRootComponent());
    DetectionWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
    DetectionWidgetComp->SetDrawSize(FVector2D(160.f, 18.f));
    DetectionWidgetComp->SetRelativeLocation(FVector(0, 0, 120.f));
}

void AEnemyCharacter::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning, TEXT("Enemy BeginPlay"));

    PlayerRef = Cast<ALightAwarenessDemoCharacter>(
        UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)
    );

    UE_LOG(LogTemp, Warning, TEXT("PlayerRef = %s"), PlayerRef ? TEXT("VALID") : TEXT("NULL"));


    // Detection widget 처리
    if (UUserWidget* W = DetectionWidgetComp->GetWidget())
    {
        if (UWBP_DetectionBar* Bar = Cast<UWBP_DetectionBar>(W))
        {
            Bar->OwnerEnemy = this;
            UE_LOG(LogTemp, Warning, TEXT("OwnerEnemy Set OK"));
        }
    }

    // Timer 시작
    const float Interval = (Config ? Config->SampleInterval : 0.05f);
    GetWorldTimerManager().SetTimer(
        SampleTimer,
        this,
        &AEnemyCharacter::SampleAndIntegrate,
        Interval,
        true
    );

    AEnemyAIController* AIC = Cast<AEnemyAIController>(GetController());
    if (AIC && PlayerRef)
    {
        AIC->SetPlayerRef(PlayerRef);
    }

    if (AEnemyAIController* EnemyAIC = Cast<AEnemyAIController>(GetController()))
    {
        EnemyAIC->SetPlayerRef(PlayerRef);

        if (EnemyAIC->BlackboardComp && PlayerRef)
        {
            EnemyAIC->BlackboardComp->SetValueAsObject(TEXT("PlayerActor"), PlayerRef);
            UE_LOG(LogTemp, Warning, TEXT("AIController: PlayerRef SET to Blackboard"));
        }
    }
}

void AEnemyCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    float S = GetVelocity().Size();
    UE_LOG(LogTemp, Warning, TEXT("Enemy Speed = %.2f  Velocity = %s"),
        S, *GetVelocity().ToString());
}

void AEnemyCharacter::SampleAndIntegrate()
{
    if (!PlayerRef || !Config) return;

    ALightAwarenessDemoCharacter* Player = Cast<ALightAwarenessDemoCharacter>(PlayerRef);
    if (!Player) return;

    float Dist = FVector::Distance(PlayerRef->GetActorLocation(), GetActorLocation());
    float Proximity = FMath::Clamp(1.f - (Dist / Config->DetectRange), 0.f, 1.f);

    float LightLvl = Player->CalculateLightLevel();

    AAIController* AAIC = Cast<AAIController>(GetController());
    bool bHasLOS = false;

    if (AAIC && PlayerRef)
    {
        bHasLOS = AAIC->LineOfSightTo(PlayerRef);
    }
    float LOSFactor = bHasLOS ? 1.0f : 0.3f;

    float DetectionChance =
        (0.8f * LightLvl + 0.2f * Proximity) * LOSFactor;
    FString Msg = FString::Printf(TEXT("Light=%.2f  Prox=%.2f  LOS=%d  Detect=%.1f"),
        LightLvl, Proximity, bHasLOS ? 1 : 0, DetectionLevel);

    GEngine->AddOnScreenDebugMessage(1, 0.0f, FColor::Yellow, Msg);

    DetectionLevel = FMath::Clamp(
        DetectionLevel + DetectionChance * Config->GainScale * Config->SampleInterval,
        0.f,
        100.f
    );


    if (DetectionChance < 0.1f)
    {
        float DecayRate = bHasLOS ? Config->DecayPerSec * 0.3f : Config->DecayPerSec;
        DetectionLevel = FMath::Clamp(
            DetectionLevel - DecayRate * Config->SampleInterval,
            0.f,
            100.f
        );
    }

    const bool bWasDetected = bPlayerDetected;
    const bool bNowDetected = (DetectionLevel >= AwarenessThreshold);

    if (bNowDetected && !bWasDetected)
    {
        bPlayerDetected = true;
        UE_LOG(LogTemp, Warning, TEXT("Player DETECTED!"));
    }
    else if (!bNowDetected && bWasDetected && DetectionLevel < AwarenessThreshold * 0.6f)
    {
        bPlayerDetected = false;
    }

    AAIController* AIC = Cast<AAIController>(GetController());
    if (!AIC) return;

    UBlackboardComponent* BB = AIC->FindComponentByClass<UBlackboardComponent>();
    if (!BB) return;

    BB->SetValueAsBool(TEXT("IsDetected"), bPlayerDetected);

    if (bPlayerDetected)
    {
        BB->SetValueAsObject(TEXT("PlayerActor"), PlayerRef);

        BB->SetValueAsVector(TEXT("LastKnownLocation"), PlayerRef->GetActorLocation());
    }
}

void AEnemyCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);

    UE_LOG(LogTemp, Warning, TEXT("Enemy PossessedBy"));


        
}
