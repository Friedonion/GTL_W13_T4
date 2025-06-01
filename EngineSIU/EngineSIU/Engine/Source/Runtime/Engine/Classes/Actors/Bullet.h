#pragma once
#include "GameFramework/Actor.h"

class UStaticMesh;
class UStaticMeshComponent;
class UProjectileMovementComponent;

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

private:
    UPROPERTY(EditAnywhere, UStaticMeshComponent*, StaticMeshComponent,)
    UPROPERTY(EditAnywhere, UStaticMesh*, StaticMesh, )

    // 총알 이동 (속도 궤적) 용 컴포넌트
    UProjectileMovementComponent* ProjectileMovement = nullptr;

private:
    UPROPERTY(EditAnywhere, float, InitialSpeed, )
    UPROPERTY(EditAnywhere, float, MaxSpeed, )
    UPROPERTY(EditAnywhere, float, Gravity, )
    UPROPERTY(EditAnywhere, FVector, Velocity, )
    UPROPERTY(EditAnywhere, float, ProjectileLifetime, )
    UPROPERTY(EditAnywhere, float, AccumulatedTime, )

};

