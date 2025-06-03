#include "Bullet.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ProjectileMovementComponent.h"

#include "Engine/FObjLoader.h"
#include "Enemy.h"

#include "PhysicsManager.h"
#include "PhysicsEngine/BodyInstance.h"

#include "Particles/ParticleSystem.h"

#include "Classes/Engine/AssetManager.h"
#include "Particles/ParticleSystemComponent.h"

ABullet::ABullet()
    : StaticMeshComponent(nullptr)
    , ProjectileMovement(nullptr)
    , InitialSpeed(100.f)
    , MaxSpeed(100.f)
    , Velocity(FVector::ZeroVector)
    , ProjectileLifetime(10.f)
    , Gravity(0.f)
    , AccumulatedTime(0.f)
    , ParticleSystemComponent(nullptr)
    , ParticleSystem(nullptr)
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
    NewActor->ParticleSystemComponent = ParticleSystemComponent;
    NewActor->ParticleSystem = ParticleSystem;

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

    // TO-DO: Muzzle 위치에 맞게 수정 필요
    SetActorLocation(Owner->GetActorLocation() + Owner->Direction.ToVector().GetSafeNormal() * 30.f);
    SetActorRotation(Owner->Direction);

    StaticMeshComponent->bSimulate = true;
    StaticMeshComponent->CreatePhysXGameObject();

    StaticMeshComponent->BodyInstance->CollisionEnabled = ECollisionEnabled::QueryOnly;
    StaticMeshComponent->BodyInstance->OwnerActor = this;
    PxRigidDynamic* RigidBody = StaticMeshComponent->BodyInstance->BIGameObject->DynamicRigidBody;

    RigidBody->setLinearVelocity(PxVec3(TempVelocity.X, TempVelocity.Y, TempVelocity.Z));

    // Begin Test
    ParticleSystem = FObjectFactory::ConstructObject<UParticleSystem>(this);
    ParticleSystem = UAssetManager::Get().GetParticleSystem(L"Contents/ParticleSystem/UParticleSystem_999");
    //ParticleSystem = UAssetManager::Get().GetParticleSystem(L"Contents/ParticleSystem/UParticleSystem_999.particlesystem");
    if (this == nullptr)
        return;
    ParticleSystemComponent = AddComponent<UParticleSystemComponent>(TEXT("ParticleSystemComp"));
    ParticleSystemComponent->SetOwner(this);
    ParticleSystemComponent->SetupAttachment(GetOwner()->GetRootComponent());
    ParticleSystemComponent->SetRelativeLocation(GetOwner()->GetActorLocation());
    ParticleSystemComponent->SetParticleSystem(ParticleSystem);
    ParticleSystemComponent->InitializeSystem();
    // End Test
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
    AccumulatedTime += DeltaTime;
    ParticleSystemComponent->TickComponent(DeltaTime);
    FVector Loc=ParticleSystemComponent->GetComponentLocation();
    UE_LOG(ELogLevel::Display, "%f, %f, %f", Loc.X, Loc.Y, Loc.Z);

    if (AccumulatedTime >= ProjectileLifetime)
    {
        Destroy();
    }
}


void ABullet::OnBulletHit(AActor* SelfActor, AActor* OtherActor)
{
    // 여기에 총알이 어딘가에 맞았을 때의 로직 (예: 파티클 생성, 스스로 파괴)
    // if (Cast<AEnemy>(OtherActor)) { /* Enemy를 맞췄을 때 특별한 처리 (선택 사항) */ }
    // else { /* 다른 것에 맞았을 때 */ }
    Destroy();
}
