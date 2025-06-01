#include "Bullet.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ProjectileMovementComponent.h"

ABullet::ABullet()
{
    // Tick 을 사용할 수 있게 허용 (이 액터가 Tick() 을 호출받도록)
    // PrimaryActorTick.bCanEverTick = true;

    // 메시 컴포넌트 생성 -> BulletMesh 에 저장 -> 루트 컴포넌트로 설정
    BulletMesh = AddComponent<UStaticMeshComponent>(TEXT("BulletMesh"));
    SetRootComponent(BulletMesh);

    // === 3. BulletMesh에 스태틱 메시 할당 (총알이 보이게 하려면 필수!) ===
    if (BulletMesh)
    {
        // "Contents/Sphere.obj"는 예시 경로이며, 실제 엔진의 에셋 로딩 시스템에 맞는 경로를 사용해야 합니다.
        // BulletMesh->SetStaticMesh(FObjManager::GetStaticMesh(L"Contents/Sphere.obj"));
        BulletMesh->SetStaticMesh(nullptr); // 임시로 nullptr로 설정하면 컴파일은 되지만 아무것도 보이지 않습니다.

    }

    // 이동 컴포넌트를 생성 -> ProjectileMovement 에 저장
    ProjectileMovement = AddComponent<UProjectileMovementComponent>(TEXT("BulletMovement"));

    if (ProjectileMovement)
    {
        ProjectileMovement->SetLifetime(3.0f);   // 총알 생명주기 기본 3초 (3초 후 파괴 로직이 작동하겠지만, Tick이 없어 파괴되지 않을 것임)
        ProjectileMovement->SetMaxSpeed(2000.0f); // 최대 속도 2000
        // 중력 값 설정: UProjectileMovementComponent에서 Velocity.Z += Gravity * DeltaTime; 이므로,
        // 만약 Z축이 '위'라면 중력이 '아래'로 작용하게 하려면 Gravity 값을 음수로 설정해야 합니다.
        // (마찬가지로 Tick이 없어 실제 움직임에 영향은 없지만, 올바른 값 설정)
        ProjectileMovement->SetGravity(-980.0f); // Z축이 '위'일 경우 음수 중력 가속도.
    }
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
    // 이 함수는 'PrimaryActorTick.bCanEverTick'이 정의되지 않아 호출되지 않을 것입니다.
    // 따라서 UProjectileMovementComponent::TickComponent 또한 자동으로 호출되지 않습니다.
}

void ABullet::FireInDirection(const FVector& InitialVelocity)
{
    if (ProjectileMovement)
    {
        // ProjectileMovement 컴포넌트가 제공하는 SetVelocity 메서드를 호출
        // ProjectileMovement 컴포넌트의 InitialVelocity를 설정합니다.
        // (마찬가지로 Tick이 없어 실제 움직임에 영향은 없지만, 값은 설정됩니다.)
        ProjectileMovement->SetVelocity(InitialVelocity);
    }
}
