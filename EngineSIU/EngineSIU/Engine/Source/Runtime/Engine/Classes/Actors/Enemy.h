#pragma once
#include "GameFramework/Actor.h"
#include "Core/TimerManager.h"
#include "PhysicsManager.h"

class USkeletalMeshComponent;
class UStaticMeshComponent;
class USkeletalMesh;
class UStaticMesh;
class ACharacter;
class ABullet;

class UPrimitiveComponent;
class USocketComponent;
struct GameObject;

class UBodySetup;
struct FBodyInstance;

struct FConstraintSetup;
struct FConstraintInstance;

class UParticleSystem;
class UParticleSystemComponent;

DECLARE_DELEGATE(FOnEnemyDestroy)

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
    UPROPERTY(VisibleAnywhere, bool, bRagDollCreated, )
    UPROPERTY(EditAnywhere, FVector, MuzzleOffset, = FVector(0,-4,150))

    UPROPERTY(EditAnywhere, UStaticMesh*, Weapon, )
    UPROPERTY(EditAnywhere, UStaticMeshComponent*, StaticMeshComponent, )
    UPROPERTY(VisibleAnywhere, USocketComponent*, SocketComponent, )
    // Alive를 어떻게 정의를 하는게 맞을까
    // State를 아래와 같이 나눌 예정
    // None
    // 

    // Begin Test
    void CreateCollisionShapes();
    void CreateCollisionBox_Body_Internal(float InCenterZOffsetFromActorBase, FVector InFullSize, FName& BoneName);
    void CreateCollisionConstraint_Internal(const TArray<UBodySetup*>& InBodySetups);
    void DestroyCollisions();

    void Die();

    // FIX-ME
    FString GetCleanBoneName(const FString& InFullName);

    void HandleCollision(GameObject* HitGameObject, AActor* SelfActor, AActor* OtherActor);

    TArray<FBodyInstance*> BodyInstances;
    TArray<UBodySetup*> BodySetups;

    TArray<FConstraintInstance*> ConstraintInstances;
    TArray<FConstraintSetup*> ConstraintSetups;
    // End Test

    TArray<GameObject*> CollisionRigidBodies;

    void DelayedDestroy();

    FTimerHandle DestroyDelayTimerHandle;
    FTimerHandle DestroyParticleTimerHandle;

    void DestoryParticle();

    void ApplyRagdollImpulse(ECollisionPart HitPart, const FVector& ImpulseDirection, float ImpulseMagnitude);
    GameObject* GetRagdollBodyPartByIndex(int32 BodyIndex); // 헬퍼 함수 (선택 사항)
    GameObject* GetRandomLegRagdollBodyPart();              // Leg 랜덤 선택 함수
public:
    UPROPERTY(VisibleAnywhere, FRotator, Direction, )

private:
    UPROPERTY(EditAnywhere, UParticleSystem*, ParticleSystem, )
    UPROPERTY(EditAnywhere, UParticleSystemComponent*, ParticleSystemComponent, )
public:
    virtual void RegisterLuaType(sol::state& Lua) override; // Lua에 클래스 등록해주는 함수.

    virtual bool BindSelfLuaProperties() override; // LuaEnv에서 사용할 멤버 변수 등록 함수.

    UPROPERTY(
        EditAnywhere,
        FVector,
        PatrolA,
        = FVector(200, 0, 0)
    )

    UPROPERTY(
        EditAnywhere,
        FVector,
        PatrolB,
        = FVector(-200, 0, 0)
    )

    UPROPERTY(
        EditAnywhere,
        FVector,
        PatrolStartLocation,
        = FVector(0, 0, 0)
    )

    UPROPERTY(
        EditAnywhere,
        FVector,
        MoveDirection,
        = FVector(1, 0, 0)
    )

    UPROPERTY(
        EditAnywhere,
        float,
        MoveSpeed,
        = 100.f;
    )

    enum
    {
        Patrol,
        Attack
    } State;

    FOnEnemyDestroy OnEnemyDestroy;
};

