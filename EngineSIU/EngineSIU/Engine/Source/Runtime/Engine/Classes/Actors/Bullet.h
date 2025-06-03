#pragma once
#include "GameFramework/Actor.h"

class UStaticMesh;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class UParticleSystem;
class UParticleSystemComponent;

class ABullet : public AActor
{
    DECLARE_CLASS(ABullet, AActor);

public:
    ABullet();
    virtual ~ABullet();


    virtual UObject* Duplicate(UObject* InOuter) override;
    virtual void Tick(float DeltaTime) override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Destroyed() override;
    
    void SetVelocity(const FVector& InVelocity)
    {
        Velocity = InVelocity;
    }

    void SetLifeTime(const float InLifeTime)
    {
        ProjectileLifetime = InLifeTime;
    }

    void SetVisible(bool bVisible)
    {
        this->bVisible = bVisible;
    }

private:
    UPROPERTY(EditAnywhere, UStaticMeshComponent*, StaticMeshComponent,)
    UPROPERTY(EditAnywhere, UStaticMesh*, StaticMesh, )

    // 총알 이동 (속도 궤적) 용 컴포넌트
    UProjectileMovementComponent* ProjectileMovement = nullptr;

    void OnBulletHit(AActor* SelfActor, AActor* OtherActor);

private:
    UPROPERTY(EditAnywhere, float, InitialSpeed, )
    UPROPERTY(EditAnywhere, float, MaxSpeed, )
    UPROPERTY(EditAnywhere, float, Gravity, )
    UPROPERTY(EditAnywhere, FVector, Velocity, )
    UPROPERTY(EditAnywhere, float, ProjectileLifetime, )
    UPROPERTY(EditAnywhere, float, AccumulatedTime, )
    UPROPERTY(EditAnywhere, bool, bVisible,)

    //FVector BeginLocation = FVector::ZeroVector;
    //FRotator BeginRotation = FRotator::ZeroRotator;


private:
    UPROPERTY(EditAnywhere, UParticleSystem*, ParticleSystem, )
    UPROPERTY(EditAnywhere, UParticleSystemComponent*, ParticleSystemComponent, )
};

