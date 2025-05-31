#include "Bullet.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ProjectileMovementComponent.h"

ABullet::ABullet()
{
    // Tick 을 사용할 수 있게 허용 (이 액터가 Tick() 을 호출받도록)
    //PrimaryActorTick.bCanEverTick = true;

    // 메시 컴포넌트 생성 -> BulletMesh 에 저장 -> 루트 컴포넌트로 설정
    BulletMesh = AddComponent<UStaticMeshComponent>(TEXT("BulletMesh"));
    SetRootComponent(BulletMesh);

    // 이동 컴포넌트를 생성 -> ProjectileMovement 에 저장
    ProjectileMovement = AddComponent<UProjectileMovementComponent>(TEXT("BulletMovement"));
}

ABullet::~ABullet()
{
}

void ABullet::BeginPlay()
{
    Super::BeginPlay();
}

void ABullet::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void ABullet::FireInDirection(const FVector& InitialVelocity)
{
    if (ProjectileMovement)
    {
        // ProjectileMovement 컴포넌트가 제공하는 SetVelocity 메서드를 호출
        ProjectileMovement->SetVelocity(InitialVelocity);
    }
}
