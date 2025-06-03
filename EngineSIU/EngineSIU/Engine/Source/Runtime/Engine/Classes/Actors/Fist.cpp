#include "Fist.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/FObjLoader.h"
#include "PhysicsManager.h"
#include "PhysicsEngine/BodyInstance.h"
#include "Engine/Engine.h"


AFist::AFist()
{
    
}

void AFist::PostSpawnInitialize()
{
}

UObject* AFist::Duplicate(UObject* InOuter)
{
    return Super::Duplicate(InOuter);
}

void AFist::BeginPlay()
{


    if (bSpawned)
    {
        SpawnObject();
    }
}

void AFist::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bSpawned)
    {
        WaitTime -= DeltaTime;
        if (WaitTime <= 0.f)
        {
            bSpawned = true;
            SpawnObject();
        }
    }
    else
    {
        Lifetime -= DeltaTime;
        if (Lifetime <= 0.f)
        {
            Destroy();
        }
    }
}

void AFist::Destroyed()
{
    GEngine->PhysicsManager->DestroyGameObject(StaticMeshComponent->BodyInstance->BIGameObject);
    Super::Destroyed();
}

void AFist::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

void AFist::SpawnObject()
{
    StaticMeshComponent = AddComponent<UStaticMeshComponent>("Fist");
    StaticMeshComponent->SetupAttachment(RootComponent);

    UStaticMesh* Mesh = nullptr;
    if (bVisible)
    {
        Mesh = FObjManager::GetStaticMesh(L"Contents/Bullet/Bullet.obj");
    }
    else
    {
        Mesh = FObjManager::GetStaticMesh(L"Contents/EmptyObject/EmptyObject.obj");
    }
    StaticMeshComponent->SetStaticMesh(Mesh);

    // 크기를 강제로 설정
    StaticMeshComponent->AABB = FBoundingBox(FVector(-0.5f, -0.5f, -0.5f), FVector(0.5f, 0.5f, 0.5f));

    StaticMeshComponent->RigidBodyType = ERigidBodyType::DYNAMIC;
    StaticMeshComponent->bSimulate = true;
    StaticMeshComponent->bApplyGravity = false;

    StaticMeshComponent->BodyInstance = new FBodyInstance(StaticMeshComponent);

    StaticMeshComponent->BodyInstance->bSimulatePhysics = StaticMeshComponent->bSimulate;
    StaticMeshComponent->BodyInstance->bEnableGravity = StaticMeshComponent->bApplyGravity;

    FVector Pos = Shooter->GetActorLocation();
    Pos += Shooter->GetActorForwardVector() * 20;
    SetActorLocation(Pos);
    SetActorRotation(Shooter->GetActorRotation());

    StaticMeshComponent->CreatePhysXGameObject();
    //GameObject = GEngine->PhysicsManager->CreateGameObject(Position, Direction, StaticMeshComponent->BodyInstance, StaticMeshComponent->GetBodySetup(), ERigidBodyType::DYNAMIC);

    StaticMeshComponent->BodyInstance->CollisionEnabled = ECollisionEnabled::QueryOnly;
    StaticMeshComponent->BodyInstance->OwnerActor = this;
    PxRigidDynamic* RigidBody = StaticMeshComponent->BodyInstance->BIGameObject->DynamicRigidBody;

    FVector Velocity = GetOwner()->GetActorForwardVector();
    RigidBody->setLinearVelocity(PxVec3(Velocity.X, Velocity.Y, Velocity.Z) * InitialSpeed);
}
