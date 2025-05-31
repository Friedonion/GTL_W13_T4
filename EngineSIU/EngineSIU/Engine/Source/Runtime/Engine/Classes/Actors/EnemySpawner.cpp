#include "EnemySpawner.h"
#include "Enemy.h"
#include "Engine/EditorEngine.h"
//#include "Actors/Character.h"

AEnemySpawner::AEnemySpawner()
    : CurrentSpawnTimer(0.f)
    , SpawnInterval(3.f)
    , bShouldSpawn(false)
    , Character(nullptr)
{
}

UObject* AEnemySpawner::Duplicate(UObject* InOuter)
{
    ThisClass* NewActor = Cast<ThisClass>(Super::Duplicate(InOuter));
    NewActor->SpawnInterval = SpawnInterval; 
    NewActor->bShouldSpawn = bShouldSpawn;
    NewActor->CurrentSpawnTimer = CurrentSpawnTimer;

    return NewActor;
}

void AEnemySpawner::BeginPlay()
{
    Super::BeginPlay();

    CurrentSpawnTimer = 0.0f;

    if (SpawnInterval <= 0.f)
    {
        SpawnInterval = 3.0f;
    }
}

void AEnemySpawner::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    CalculateTimer(DeltaTime);
    if (!bShouldSpawn) return;

    if (CanSpawn())
    {
        Spawn();
    }
}

void AEnemySpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

void AEnemySpawner::Spawn()
{
    UWorld* World = GEngine->ActiveWorld;
    AEnemy* SpawnedEnemy = nullptr;

    SpawnedEnemy = World->SpawnActor<AEnemy>();
    SpawnedEnemy->SetActorLabel(TEXT("OBJ_ENEMY"));
    SpawnedEnemy->SetActorLocation(this->GetActorLocation());

    bShouldSpawn = false;
}

bool AEnemySpawner::CanSpawn()
{
    FVector2D CharacterToSpawner = /*FVector2D(this->GetActorLocation().X, this->GetActorLocation().Y) - FVector2D(Character->GetActorLocation().X, Character->GetActorLocation().Y)*/FVector2D();
    FVector2D CharacterDirection = /* 이거는 카메라 방향으로 하는게 좋을 것 같음*/FVector2D();

    if (FVector2D::DotProduct(CharacterToSpawner, CharacterDirection) <= 0.f)
    {
        return true;
    }

    return false;
}

void AEnemySpawner::CalculateTimer(float DeltaTime)
{
    CurrentSpawnTimer += DeltaTime;

    if (CurrentSpawnTimer >= SpawnInterval)
    {
        CurrentSpawnTimer = 0.f;
        
        bShouldSpawn = true;
    }
}
