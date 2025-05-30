#pragma once
#include "GameFramework/Actor.h"
class AEnemySpawner : public AActor
{
    DECLARE_CLASS(AEnemySpawner, AActor)

public:
    AEnemySpawner() = default;

    virtual UObject* Duplicate(UObject* InOuter) override;

    virtual void Tick(float DeltaTime) override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Destroyed() override;

private:
    void Spawn();
    // delegate로 체크를 해야 하나?
    // CheckCondition
private:
    UPROPERTY(EditAnywhere, float, SpawnInterval, = 3.f)
    UPROPERTY(VisibleAnywhere, bool, bCanSpawn, = false)
};

