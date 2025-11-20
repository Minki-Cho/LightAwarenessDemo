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
}

void AEnemyCharacter::SampleAndIntegrate()
{
    if (!PlayerRef || !Config) return;

    ALightAwarenessDemoCharacter* Player = Cast<ALightAwarenessDemoCharacter>(PlayerRef);
    if (!Player) return;

    // ----------------------------
    // 1) 거리 기반 Proximity
    // ----------------------------
    float Dist = FVector::Distance(PlayerRef->GetActorLocation(), GetActorLocation());
    float Proximity = FMath::Clamp(1.f - (Dist / Config->DetectRange), 0.f, 1.f);

    // ----------------------------
    // 2) 빛 기반 감지 Light Level
    // ----------------------------
    float LightLvl = Player->CalculateLightLevel();

    // ----------------------------
    // 3) 시야(LOS) 체크
    // ----------------------------
    AAIController* AAIC = Cast<AAIController>(GetController());
    bool bHasLOS = false;

    if (AAIC && PlayerRef)
    {
        bHasLOS = AAIC->LineOfSightTo(PlayerRef);
    }
    float LOSFactor = bHasLOS ? 1.0f : 0.3f;

    // ----------------------------
    // 4) 최종 DetectionChance (가중 결합)
    // ----------------------------
    float DetectionChance =
        (0.8f * LightLvl + 0.2f * Proximity) * LOSFactor;
    FString Msg = FString::Printf(TEXT("Light=%.2f  Prox=%.2f  LOS=%d  Detect=%.1f"),
        LightLvl, Proximity, bHasLOS ? 1 : 0, DetectionLevel);

    GEngine->AddOnScreenDebugMessage(1, 0.0f, FColor::Yellow, Msg);
    // 감지 게이지 증가
    DetectionLevel = FMath::Clamp(
        DetectionLevel + DetectionChance * Config->GainScale * Config->SampleInterval,
        0.f,
        100.f
    );

    // 감지 게이지 감소
    if (DetectionChance < 0.1f)
    {
        float DecayRate = bHasLOS ? Config->DecayPerSec * 0.3f : Config->DecayPerSec;
        DetectionLevel = FMath::Clamp(
            DetectionLevel - DecayRate * Config->SampleInterval,
            0.f,
            100.f
        );
    }

    // ----------------------------
    // 5) 감지 상태 전환
    // ----------------------------
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

    // ----------------------------
    // 6) Blackboard 업데이트 (MoveTo용)
    // ----------------------------
    AAIController* AIC = Cast<AAIController>(GetController());
    if (!AIC) return;

    UBlackboardComponent* BB = AIC->FindComponentByClass<UBlackboardComponent>();
    if (!BB) return;

    BB->SetValueAsBool(TEXT("IsDetected"), bPlayerDetected);

    // 감지 중일 때만 PlayerActor 계속 갱신
    if (bPlayerDetected)
    {
        BB->SetValueAsObject(TEXT("PlayerActor"), PlayerRef);

        // --- NEW: 마지막 본 위치 저장 (Investigate 용)
        BB->SetValueAsVector(TEXT("LastKnownLocation"), PlayerRef->GetActorLocation());
    }
}

void AEnemyCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);

    UE_LOG(LogTemp, Warning, TEXT("Enemy PossessedBy"));


        
}
