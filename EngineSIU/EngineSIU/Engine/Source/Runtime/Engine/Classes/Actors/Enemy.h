#pragma once
#include "GameFramework/Actor.h"

class USkeletalMeshComponent;
class USkeletalMesh;
class ACharacter;
class ABullet;

class UPrimitiveComponent;
class UBodySetup;
struct FBodyInstance;

class AEnemy : public AActor
{
    DECLARE_CLASS(AEnemy, AActor)

public:
    AEnemy();

    virtual UObject* Duplicate(UObject* InOuter) override;

    virtual void Tick(float DeltaTime) override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual bool Destroy() override;
    virtual void Destroyed() override;

    void SetRandomFireInterval();

private:
    void Fire();
    void CalculateTimer(float DeltaTime);
    // SpawnProjectile

private:
    float CurrentFireTimer;
    bool bRagDollCreated;

    UPROPERTY(VisibleAnywhere, USkeletalMeshComponent*, SkeletalMeshComponent,)
    UPROPERTY(VisibleAnywhere, USkeletalMesh*, SkeletalMesh, )
    UPROPERTY(VisibleAnywhere, ACharacter*, Character, )
    UPROPERTY(EditAnywhere, float, FireInterval,)
    UPROPERTY(VisibleAnywhere, bool, bShouldFire, )
    UPROPERTY(VisibleAnywhere, bool, bIsAlive, )
    //UPROPERTY(VisibleAnywhere, UCapsuleComponent*, CapsuleComponent, )


    // Begin Test
    void CreateCollisionCapsule();

    FBodyInstance* BodyInstance = nullptr;
    UBodySetup* BodySetup = nullptr;
    UPROPERTY(VisibleAnywhere, UPrimitiveComponent*, PrimitiveComponent, )
    // End Test


public:
    UPROPERTY(VisibleAnywhere, FRotator, Direction, )
};

