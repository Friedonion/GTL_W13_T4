#include "Bullet.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ProjectileMovementComponent.h"

#include "Engine/FObjLoader.h"
#include "Enemy.h"

ABullet::ABullet()
    : StaticMeshComponent(nullptr)
    , ProjectileMovement(nullptr)
    , InitialSpeed(10.f)
    , MaxSpeed(10.f)
    , Velocity(FVector::ZeroVector)
    , ProjectileLifetime(2.f)
    , Gravity(1.f)
    , AccumulatedTime(0.f)
{
}

ABullet::~ABullet()
{
}


UObject* ABullet::Duplicate(UObject* InOuter)
{
    ThisClass* NewActor = Cast<ThisClass>(Super::Duplicate(InOuter));
    NewActor->StaticMeshComponent = StaticMeshComponent;
    NewActor->ProjectileMovement = ProjectileMovement;
    NewActor->InitialSpeed = InitialSpeed;

    return NewActor;
}

void ABullet::BeginPlay()
{
    Super::BeginPlay();

    StaticMeshComponent = AddComponent<UStaticMeshComponent>(TEXT("BulletMesh"));
    SetRootComponent(StaticMeshComponent);

    StaticMesh = FObjManager::GetStaticMesh(L"Contents/Bullet/Bullet.obj");
    StaticMeshComponent->SetStaticMesh(StaticMesh);

    ProjectileMovement = AddComponent<UProjectileMovementComponent>(TEXT("BulletMovement"));
    if (ProjectileMovement)
    {
        ProjectileMovement->SetInitialSpeed(InitialSpeed);
        ProjectileMovement->SetMaxSpeed(MaxSpeed);
        ProjectileMovement->SetVelocity(GetOwner()->GetActorForwardVector());
        
        ProjectileMovement->SetGravity(Gravity);
    }

	SetActorLocation(GetOwner()->GetActorLocation());
    SetActorRotation(GetOwner()->GetRootComponent()->GetRelativeRotation());
}

void ABullet::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

void ABullet::Destroyed()
{
    Super::Destroyed();
    AEnemy* Owner = Cast<AEnemy>(GetOwner());
    Owner->Bullet = nullptr;
}

void ABullet::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}
