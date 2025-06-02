#include "Bullet.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ProjectileMovementComponent.h"

#include "Engine/FObjLoader.h"
#include "Enemy.h"

#include "PhysicsManager.h"
#include "PhysicsEngine/BodyInstance.h"

ABullet::ABullet()
    : StaticMeshComponent(nullptr)
    , ProjectileMovement(nullptr)
    , InitialSpeed(100.f)
    , MaxSpeed(100.f)
    , Velocity(FVector::ZeroVector)
    , ProjectileLifetime(10.f)
    , Gravity(0.f)
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
    NewActor->MaxSpeed = MaxSpeed;
    NewActor->Velocity = Velocity;
    NewActor->ProjectileLifetime = ProjectileLifetime;
    NewActor->Gravity = Gravity;
    NewActor->AccumulatedTime = AccumulatedTime;

    return NewActor;
}

void ABullet::BeginPlay()
{
    Super::BeginPlay();

    AEnemy* Owner = Cast<AEnemy>(GetOwner());
    FVector TempVelocity;
    StaticMeshComponent = AddComponent<UStaticMeshComponent>(TEXT("BulletMesh"));
    SetRootComponent(StaticMeshComponent);

    StaticMesh = FObjManager::GetStaticMesh(L"Contents/Bullet/Bullet.obj");
    StaticMeshComponent->SetStaticMesh(StaticMesh);

    ProjectileMovement = AddComponent<UProjectileMovementComponent>(TEXT("BulletMovement"));

    TempVelocity = Owner->Direction.ToVector() * InitialSpeed;
	SetActorLocation(Owner->GetActorLocation());
    SetActorRotation(Owner->Direction);

    StaticMeshComponent->bSimulate = true;
    StaticMeshComponent->CreatePhysXGameObject();

    StaticMeshComponent->BodyInstance->CollisionEnabled = ECollisionEnabled::QueryOnly;
    PxRigidDynamic* RigidBody = StaticMeshComponent->BodyInstance->BIGameObject->DynamicRigidBody;

    RigidBody->setLinearVelocity(PxVec3(TempVelocity.X, TempVelocity.Y, TempVelocity.Z));
}

void ABullet::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

void ABullet::Destroyed()
{
    Super::Destroyed();
    
}

void ABullet::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    AccumulatedTime += DeltaTime;

    if (AccumulatedTime >= ProjectileLifetime)
    {
        Destroy();
    }
}
