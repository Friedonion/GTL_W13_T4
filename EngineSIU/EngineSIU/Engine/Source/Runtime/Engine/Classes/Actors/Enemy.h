#pragma once
#include "GameFramework/Actor.h"

class USkeletalMeshComponent;
class USkeletalMesh;
class ACharacter;
class ABullet;

class UPrimitiveComponent;
class UBodySetup;
struct FBodyInstance;
struct GameObject;

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
    
    void SetLuaToPlayAnim();

private:
    float CurrentFireTimer;
    bool bCapsuleCreated;
    bool bRagDollCreated;

    UPROPERTY(VisibleAnywhere, USkeletalMeshComponent*, SkeletalMeshComponent,)
    UPROPERTY(VisibleAnywhere, USkeletalMesh*, SkeletalMesh, )
    UPROPERTY(VisibleAnywhere, ACharacter*, Character, )
    UPROPERTY(EditAnywhere, float, FireInterval,)
    UPROPERTY(VisibleAnywhere, bool, bShouldFire, )
    UPROPERTY(VisibleAnywhere, bool, bIsAlive, ) // 아직 Destroy되지 않았지만 Fire()하지 않아야 하므로

    // Begin Test
    void CreateCollisionCapsule();
    void DestroyCollisionCapsule();

    FBodyInstance* BodyInstance;
    UBodySetup* BodySetup;
    // End Test

    GameObject* Capsule;

public:
    UPROPERTY(VisibleAnywhere, FRotator, Direction, )
};

