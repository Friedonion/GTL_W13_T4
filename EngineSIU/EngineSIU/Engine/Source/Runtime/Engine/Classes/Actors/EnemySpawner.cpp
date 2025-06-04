#include "EnemySpawner.h"
#include "Enemy.h"
#include "Engine/EditorEngine.h"
#include "Actors/Player/Player.h"

// 스폰한 액터가 없어지고 나면 다시 카운트다운하도록 변경함.
AEnemySpawner::AEnemySpawner()
    : CurrentSpawnTimer(0.f)
    , SpawnInterval(1.f)
    , bShouldSpawn(false)
    , Character(nullptr)
    , SpawnedEnemy(nullptr)
{
}

UObject* AEnemySpawner::Duplicate(UObject* InOuter)
{
    ThisClass* NewActor = Cast<ThisClass>(Super::Duplicate(InOuter));
    NewActor->CurrentSpawnTimer = CurrentSpawnTimer;
    NewActor->SpawnInterval = SpawnInterval; 
    NewActor->bShouldSpawn = bShouldSpawn;

    NewActor->Character = Character;
    NewActor->SpawnedEnemy = SpawnedEnemy;

    return NewActor;
}

void AEnemySpawner::BeginPlay()
{
    Super::BeginPlay();

    CurrentSpawnTimer = 0.0f;

    PatrolDistance = GetActorScale().X;
    SpawnInterval = GetActorScale().Y;
    InitSpeed = GetActorScale().Z;

    PatrolDirection = GetActorForwardVector();

    bShouldSpawn = true;
}

void AEnemySpawner::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bFirstSpawn)
    {
        bFirstSpawn = false;
        Spawn();
    }

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

void AEnemySpawner::Destroyed()
{
    Super::Destroyed();

}

// Spawn 시간을 판단
void AEnemySpawner::CalculateTimer(float DeltaTime)
{
    CurrentSpawnTimer += DeltaTime;

    if (CurrentSpawnTimer >= SpawnInterval)
    {
        CurrentSpawnTimer = 0.f;
        bShouldSpawn = true;
    }
}

// Player의 방향을 판단
bool AEnemySpawner::CanSpawn()
{
    FVector PlayerLocation3D = GEngine->ActiveWorld->GetMainPlayer()->GetActorLocation();   
    FVector PlayerDirection3D = GEngine->ActiveWorld->GetMainPlayer()->GetActorForwardVector();
    if (APlayerCharacter* Player = Cast<APlayerCharacter>(GEngine->ActiveWorld->GetMainPlayer()))
    {
        PlayerDirection3D = Player->GetHeadRotation().RotateVector(FVector(1, 0, 0));
    }

    FVector2D PlayerToSpawner2D = FVector2D(this->GetActorLocation().X, this->GetActorLocation().Y) - FVector2D(PlayerLocation3D.X, PlayerLocation3D.Y);
    FVector2D PlayerDirection2D = FVector2D(PlayerDirection3D.X, PlayerDirection3D.Y);

    if (SpawnedEnemy != nullptr) 
        return false; 

    float result = FVector2D::DotProduct(PlayerToSpawner2D, PlayerDirection2D);
    if (result > 0.f)
    {
        return false;
    }

    return true;
}

void AEnemySpawner::Spawn()
{
    UWorld* World = GEngine->ActiveWorld;

    if (SpawnedEnemy != nullptr) return;

    SpawnedEnemy = World->SpawnActor<AEnemy>();
    SpawnedEnemy->SetActorLabel(TEXT("OBJ_ENEMY"));
    SpawnedEnemy->SetOwner(this);
    SpawnedEnemy->PatrolA = GetActorLocation() + PatrolDirection * PatrolDistance;
    SpawnedEnemy->PatrolB = GetActorLocation() - PatrolDirection * PatrolDistance;

    uintptr_t Address = reinterpret_cast<uintptr_t>(SpawnedEnemy);
    SpawnedEnemy->MoveSpeed = InitSpeed; // 1~5

    SpawnedEnemy->OnEnemyDestroy.BindLambda([this]()
        {
            SpawnedEnemy = nullptr;
            bShouldSpawn = false;
            CurrentSpawnTimer = 0.f; // 스폰 타이머 초기화
        });

    bShouldSpawn = false;
}
