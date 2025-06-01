#pragma once
#include "GameFramework/Actor.h"

class USkeletalMeshComponent;
class USkeletalMesh;
class ACharacter;
class ABullet;

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
    void CalculateTimer(float DeltaTime);
    // SpawnProjectile

private:
    UPROPERTY(VisibleAnywhere, USkeletalMeshComponent*, SkeletalMeshComponent,)
    UPROPERTY(VisibleAnywhere, USkeletalMesh*, SkeletalMesh, )
    UPROPERTY(VisibleAnywhere, ACharacter*, Character, )
    UPROPERTY(EditAnywhere, float, FireInterval,)
    UPROPERTY(VisibleAnywhere, bool, bShouldFire, )
    float CurrentFireTimer;

public:
    UPROPERTY(VisibleAnywhere, ABullet*, Bullet, )

};

