#pragma once
#include "GameFramework/Actor.h"
#include "Core/TimerManager.h"

class USkeletalMeshComponent;
class USkeletalMesh;
class ACharacter;
class ABullet;

class UPrimitiveComponent;
struct GameObject;

class UBodySetup;
struct FBodyInstance;

struct FConstraintSetup;
struct FConstraintInstance;

class AEnemy : public AActor
{
    DECLARE_CLASS(AEnemy, AActor)

public:
    AEnemy();
    ~AEnemy();

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

    UPROPERTY(VisibleAnywhere, USkeletalMeshComponent*, SkeletalMeshComponent,)
    UPROPERTY(VisibleAnywhere, USkeletalMesh*, SkeletalMesh, )
    UPROPERTY(VisibleAnywhere, ACharacter*, Character, )
    UPROPERTY(EditAnywhere, float, FireInterval,)
    UPROPERTY(VisibleAnywhere, bool, bShouldFire, )
    UPROPERTY(VisibleAnywhere, bool, bIsAlive, ) // 아직 Destroy되지 않았지만 Fire()하지 않아야 하므로

    UPROPERTY(EditAnywhere, FName, MuzzleSocketName, )

    // Begin Test
    void CreateCollisionShapes();
    void CreateCollisionBox_Body_Internal(float InCenterZOffsetFromActorBase, FVector InFullSize, FName& BoneName);
    void CreateCollisionConstraint_Internal(const TArray<UBodySetup*>& InBodySetups);
    void DestroyCollisions();

    void Die();

    // FIX-ME
    FString GetCleanBoneName(const FString& InFullName);

    void HandleCollision(AActor* SelfActor, AActor* OtherActor);

    TArray<FBodyInstance*> BodyInstances;
    TArray<UBodySetup*> BodySetups;

    TArray<FConstraintInstance*> ConstraintInstances;
    TArray<FConstraintSetup*> ConstraintSetups;
    // End Test

    TArray<GameObject*> CollisionRigidBodies;

    void DelayedDestroy();
    void PeriodicAttackCheck();

    FTimerHandle DestroyDelayTimerHandle;
    FTimerHandle AttackCheckTimerHandle;

public:
    UPROPERTY(VisibleAnywhere, FRotator, Direction, )
};

