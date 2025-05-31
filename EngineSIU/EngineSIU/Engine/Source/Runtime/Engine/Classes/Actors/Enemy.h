#pragma once
#include "GameFramework/Actor.h"

class USkeletalMeshComponent;

class UStaticMeshComponent;

class AEnemy : public AActor
{
    DECLARE_CLASS(AEnemy, AActor)

public:
    AEnemy();

    virtual UObject* Duplicate(UObject* InOuter) override;

    virtual void Tick(float DeltaTime) override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Destroyed() override;

private:
    void Fire();
    // SpawnProjectile
    // ACharacter의 위치를 찾는 로직

private:
    UPROPERTY(VisibleAnywhere, USkeletalMeshComponent*, SkeletalMeshComponent, = nullptr)
    UPROPERTY(EditAnywhere, float, FireInterval, = 3.f)
};

