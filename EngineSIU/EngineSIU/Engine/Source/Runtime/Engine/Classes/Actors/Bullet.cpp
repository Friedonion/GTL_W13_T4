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

#include "Engine/Engine.h"

ABullet::ABullet()
    : StaticMeshComponent(nullptr)
    , ProjectileMovement(nullptr)
    , InitialSpeed(100.f)
    , MaxSpeed(100.f)
    , Velocity(FVector::ZeroVector)
    , ProjectileLifetime(10.f)
    , Gravity(0.f)
    , AccumulatedTime(0.f)
    , bVisible(true)
    , ParticleSystemComponent(nullptr)
    , ParticleSystem(nullptr)
    , WhizzSoundChannel(nullptr)
{
    SoundName = "whizzby";
}

ABullet::~ABullet()
{
    if (WhizzSoundChannel) {
        WhizzSoundChannel->stop();
        WhizzSoundChannel = nullptr;
    }
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
    NewActor->WhizzSoundChannel = WhizzSoundChannel;

    return NewActor;
}

void ABullet::BeginPlay()
{
    Super::BeginPlay();

    AActor* Owner = GetOwner();
    
    FVector Location = RootComponent->GetRelativeLocation();
    FRotator Rotation = RootComponent->GetRelativeRotation();
    FVector Scale = RootComponent->GetRelativeScale3D();
    StaticMeshComponent = AddComponent<UStaticMeshComponent>(TEXT("BulletMesh"));

    StaticMeshComponent->SetRelativeLocation(Location);
    StaticMeshComponent->SetRelativeRotation(Rotation);
    StaticMeshComponent->SetRelativeScale3D(FVector(10, 10, 10));

    //SetActorScale(FVector(10, 10, 10));
    if (bVisible)
    {
        StaticMesh = FObjManager::GetStaticMesh(L"Contents/Bullet/Bullet.obj");
        StaticMeshComponent->SetStaticMesh(StaticMesh);
    }
    else
    {
        StaticMesh = FObjManager::GetStaticMesh(L"Contents/EmptyObject/EmptyObject.obj");
        StaticMeshComponent->SetStaticMesh(StaticMesh);
        StaticMeshComponent->AABB = FBoundingBox(FVector(-0.5f, -0.5f, -0.5f), FVector(0.5f, 0.5f, 0.5f));
    }

    //ProjectileMovement = AddComponent<UProjectileMovementComponent>(TEXT("BulletMovement"));

    // TO-DO: Muzzle 위치에 맞게 수정 필요
    //SetActorLocation(Owner->GetActorLocation() + Owner->Direction.ToVector().GetSafeNormal() * 30.f);
    //SetActorRotation(Owner->Direction);

    StaticMeshComponent->bSimulate = true;
    StaticMeshComponent->CreatePhysXGameObject();

    StaticMeshComponent->BodyInstance->CollisionEnabled = ECollisionEnabled::QueryAndPhysics;
    StaticMeshComponent->BodyInstance->OwnerActor = this;

    StaticMeshComponent->BodyInstance->bUseCCD = true;                                        // CCD 활성화
    StaticMeshComponent->BodyInstance->bStartAwake = true;                                    // 항상 깨어있는 상태로 시작
    StaticMeshComponent->BodyInstance->PositionSolverIterationCount = 8;                     // 위치 솔버 반복 횟수 증가
    StaticMeshComponent->BodyInstance->VelocitySolverIterationCount = 4;                     // 속도 솔버 반복 횟수 증가
    PxRigidDynamic* RigidBody = StaticMeshComponent->BodyInstance->BIGameObject->DynamicRigidBody;

    FVector Velocity = GetActorForwardVector() * 1000.f;

    RigidBody->setLinearVelocity(PxVec3(Velocity.X, Velocity.Y, Velocity.Z));
    // Begin Test
    ParticleSystem = FObjectFactory::ConstructObject<UParticleSystem>(this);
    ParticleSystem = UAssetManager::Get().GetParticleSystem(L"Contents/ParticleSystem/UParticleSystem_1103");
    if (this == nullptr)
        return;
    ParticleSystemComponent = AddComponent<UParticleSystemComponent>(TEXT("ParticleSystemComp"));
    ParticleSystemComponent->SetOwner(this);
    ParticleSystemComponent->SetupAttachment(StaticMeshComponent);
    ParticleSystemComponent->SetParticleSystem(ParticleSystem);
    ParticleSystemComponent->InitializeSystem();
    // End Test
    StaticMeshComponent->BodyInstance->BIGameObject->OnHit.AddUObject(this, &ABullet::HandleCollision);



    FVector CurrentLocation = GetActorLocation();

    FMOD_VECTOR fmodPos = { CurrentLocation.X, CurrentLocation.Y, CurrentLocation.Z };
    FMOD_VECTOR fmodVel = { Velocity.X, Velocity.Y, Velocity.Z };
    \
    WhizzSoundChannel = FSoundManager::GetInstance().PlaySoundAtLocation(*SoundName.ToString(), fmodPos, fmodVel, true /*startPaused = true*/);
    if (WhizzSoundChannel) 
    {
        // 초기 3D 속성 설정 (PlaySoundAtLocation 내부에서 이미 수행했을 수 있음)
        WhizzSoundChannel->set3DAttributes(&fmodPos, &fmodVel);
        WhizzSoundChannel->setPaused(false); 
    }
}

void ABullet::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (WhizzSoundChannel) {
        WhizzSoundChannel->stop();
        WhizzSoundChannel = nullptr;
    }
    Super::EndPlay(EndPlayReason);
}

bool ABullet::Destroy()
{
    if (WhizzSoundChannel) {
        WhizzSoundChannel->stop();
        WhizzSoundChannel = nullptr;
    }
    GEngine->PhysicsManager->DestroyGameObject(StaticMeshComponent->BodyInstance->BIGameObject);
    return Super::Destroy();
}

void ABullet::Destroyed()
{
    Super::Destroyed();
}

void ABullet::HandleCollision(GameObject* HitGameObject, AActor* SelfActor, AActor* OtherActor)
{
    // 여기에 총알이 어딘가에 맞았을 때의 로직 (예: 파티클 생성, 스스로 파괴)
// if (Cast<AEnemy>(OtherActor)) { /* Enemy를 맞췄을 때 특별한 처리 (선택 사항) */ }
// else { /* 다른 것에 맞았을 때 */ }
    bDestroy = true;
}

void ABullet::Tick(float DeltaTime)
{
    AccumulatedTime += DeltaTime;
    ParticleSystemComponent->TickComponent(DeltaTime);
    FVector Loc = ParticleSystemComponent->GetComponentLocation(); // 변경값
    UE_LOG(ELogLevel::Display, "%f, %f, %f", Loc.X, Loc.Y, Loc.Z);

    FVector WorldLocation=StaticMeshComponent->GetComponentLocation();
    FSoundManager::GetInstance().PlaySoundAtLocation(*SoundName.ToString(), { WorldLocation.X , WorldLocation.Y , WorldLocation.Z });

    if (WhizzSoundChannel) {
        bool isPlaying = false;
        FMOD_RESULT result = WhizzSoundChannel->isPlaying(&isPlaying);
        if (result == FMOD_OK && isPlaying) {
            FVector CurrentLocation = GetActorLocation();
            FVector PxVelocity = FVector::ZeroVector;
            if (StaticMeshComponent && StaticMeshComponent->BodyInstance && StaticMeshComponent->BodyInstance->BIGameObject && StaticMeshComponent->BodyInstance->BIGameObject->DynamicRigidBody)
            {
                PxVec3 pxVel = StaticMeshComponent->BodyInstance->BIGameObject->DynamicRigidBody->getLinearVelocity();
                PxVelocity = FVector(pxVel.x, pxVel.y, pxVel.z);
            }

            FMOD_VECTOR fmodPos = { CurrentLocation.X, CurrentLocation.Y, CurrentLocation.Z };
            FMOD_VECTOR fmodVel = { PxVelocity.X, PxVelocity.Y, PxVelocity.Z }; 
            WhizzSoundChannel->set3DAttributes(&fmodPos, &fmodVel);
        }
        else 
        {
            WhizzSoundChannel = nullptr;
        }
    }
    if (bDestroy)
    {
        Destroy();
        return;
    }
    if (AccumulatedTime >= ProjectileLifetime)
    {
        Destroy();
    }
}
