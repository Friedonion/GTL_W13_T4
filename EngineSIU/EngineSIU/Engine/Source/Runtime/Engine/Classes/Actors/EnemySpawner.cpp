#include "EnemySpawner.h"

UObject* AEnemySpawner::Duplicate(UObject* InOuter)
{
    ThisClass* NewActor = Cast<ThisClass>(Super::Duplicate(InOuter));
    NewActor->SpawnInterval = SpawnInterval; 
    NewActor->bCanSpawn = bCanSpawn;

    return NewActor;
}

void AEnemySpawner::Tick(float DeltaTime)
{
}

void AEnemySpawner::BeginPlay()
{
}

void AEnemySpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
}

void AEnemySpawner::Destroyed()
{
}

void AEnemySpawner::Spawn()
{
    if (bCanSpawn)
    {
        //Spawn
    }
}
