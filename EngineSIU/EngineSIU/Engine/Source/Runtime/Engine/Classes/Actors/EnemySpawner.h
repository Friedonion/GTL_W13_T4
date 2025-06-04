#pragma once
#include "GameFramework/Actor.h"

class ACharacter;
class AEnemy;

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
    UPROPERTY(VisibleAnywhere, bool, bShouldSpawn,)
    UPROPERTY(VisibleAnywhere, ACharacter*, Character,)
public:
    UPROPERTY(VisibleAnywhere, AEnemy*, SpawnedEnemy,)

    //UPROPERTY(EditAnywhere, FVector, PatrolA,= FVector(1000, 0, 0))
    //UPROPERTY(EditAnywhere, FVector, PatrolB,= FVector(-1000, 0, 0))
    //UPROPERTY(EditAnywhere, float, InitSpeed, =300)
    //UPROPERTY(EditAnywhere, float, InitSpeedInterval, =100)
    // Scale.X = Patrol 거리
    // Scale.Y = interval
    // Scale.Z = InitSpeed

    float PatrolDistance;
    float SpawnInterval;
    float InitSpeed;

    FVector PatrolDirection;

    float CurrentSpawnTimer;

    bool bFirstSpawn = true; // 최초 스폰 여부
};

