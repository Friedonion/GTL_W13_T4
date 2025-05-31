#pragma once
#include "Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ProjectileMovementComponent.h"

class ABullet :    public AActor
{
    DECLARE_CLASS(ABullet, AActor);

public:
    ABullet();
    virtual ~ABullet();

    // 게임 시작 시 호출
    virtual void BeginPlay() override;
   
    // 매 프레임마다 호출
    virtual void Tick(float DeltaTime) override;

    // 이후에 총알을 특정 방향으로 발사할 때 호출할 함수
    void FireInDirection(const FVector& ShootDirection);

private:
    // 총알 시각 충돌용 메시 컴포넌트
    UStaticMeshComponent* BulletMesh = nullptr;

    // 총알 이동 (속도 궤적) 용 컴포넌트
    UProjectileMovementComponent* ProjectileMovement = nullptr;

    // 이후에 생존 시간 관리용 변수
    // float LifeSpan = 3.0f;
    // float ElapsedTime = 0.0f;
};

