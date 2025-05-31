#pragma once
#include "GameFramework/Actor.h"

class ACharacter;
class AEnemy;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnSpawnedEnemyDestroyed, const bool /*bDestroyed*/);

class AEnemySpawner : public AActor
{
    DECLARE_CLASS(AEnemySpawner, AActor)

public:
    AEnemySpawner();

    virtual UObject* Duplicate(UObject* InOuter) override;

    virtual void Tick(float DeltaTime) override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Destroyed() override;

private:
    void Spawn();
    bool CanSpawn();
    void CalculateTimer(float DeltaTime);

private:
    UPROPERTY(EditAnywhere, float, SpawnInterval,)
    UPROPERTY(VisibleAnywhere, bool, bShouldSpawn,)
    UPROPERTY(VisibleAnywhere, ACharacter*, Character,)
public:
    UPROPERTY(VisibleAnywhere, AEnemy*, SpawnedEnemy,)

    float CurrentSpawnTimer;
};

