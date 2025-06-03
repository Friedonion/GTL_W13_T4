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
    StaticMeshComponent = AddComponent<UStaticMeshComponent>("Fist");
    
    UStaticMesh* Mesh = FObjManager::GetStaticMesh(L"Contents/Bullet/Bullet.obj");
    StaticMeshComponent->SetStaticMesh(Mesh);

    // 크기를 강제로 설정
    StaticMeshComponent->AABB = FBoundingBox(FVector(-0.5f, -0.5f, -0.5f), FVector(0.5f, 0.5f, 0.5f));
    
    StaticMeshComponent->RigidBodyType = ERigidBodyType::DYNAMIC;
    StaticMeshComponent->bSimulate = true;
    StaticMeshComponent->bApplyGravity = false;

    FVector Pos = GetOwner()->GetActorLocation();
    Pos += GetOwner()->GetActorForwardVector() * 0.5;
    PxVec3 Position = PxVec3(Pos.X, Pos.Y, Pos.Z);

    StaticMeshComponent->BodyInstance = new FBodyInstance(StaticMeshComponent);

    StaticMeshComponent->BodyInstance->bSimulatePhysics = StaticMeshComponent->bSimulate;
    StaticMeshComponent->BodyInstance->bEnableGravity = StaticMeshComponent->bApplyGravity;

    GameObject = GEngine->PhysicsManager->CreateGameObject(Position, PxQuat(0,0,0,1), StaticMeshComponent->BodyInstance, StaticMeshComponent->GetBodySetup(), ERigidBodyType::DYNAMIC);

    StaticMeshComponent->BodyInstance->CollisionEnabled = ECollisionEnabled::PhysicsOnly;
    PxRigidDynamic* RigidBody = StaticMeshComponent->BodyInstance->BIGameObject->DynamicRigidBody;

    FVector Velocity = GetOwner()->GetActorForwardVector();
    RigidBody->setLinearVelocity(PxVec3(Velocity.X, Velocity.Y, Velocity.Z) * 0.3);
}

void AFist::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    Lifetime -= DeltaTime;
    if (Lifetime <= 0.f)
    {
        Destroy();
    }
}

void AFist::Destroyed()
{
    GEngine->PhysicsManager->DestroyGameObject(GameObject);
    Super::Destroyed();
}

void AFist::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}
