#include "EnemyAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Kismet/GameplayStatics.h"
#include "EnemyCharacter.h"

AEnemyAIController::AEnemyAIController()
{
    BlackboardComp = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComp"));
    BehaviorComp = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorComp"));
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    UE_LOG(LogTemp, Warning, TEXT("AIController POSSESS"));

    AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(InPawn);
    if (!Enemy)
    {
        UE_LOG(LogTemp, Error, TEXT("Pawn is not AEnemyCharacter"));
        return;
    }

    // ⭐ 플레이어 가져오기 — BT 시작 전에 반드시 세팅!
    AActor* Player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
    if (Player)
    {
        BlackboardComp->SetValueAsObject(TEXT("PlayerActor"), Player);
        UE_LOG(LogTemp, Warning, TEXT("Player set in Blackboard!"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Player FIND FAILED!"));
    }

    if (Enemy->BehaviorTree)
    {
        BlackboardComp->InitializeBlackboard(*Enemy->BehaviorTree->BlackboardAsset);
        BehaviorComp->StartTree(*Enemy->BehaviorTree);
        UE_LOG(LogTemp, Warning, TEXT("Start BT!"));
    }
}

void AEnemyAIController::SetPlayerRef(ACharacter* Player)
{
    PlayerRef = Player;

    if (BlackboardComp && Player)
    {
        BlackboardComp->SetValueAsObject(TEXT("PlayerActor"), Player);
        UE_LOG(LogTemp, Warning, TEXT("AIController: PlayerRef SET in SetPlayerRef()"));
    }
}
