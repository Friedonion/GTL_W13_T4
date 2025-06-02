#include "Enemy.h"
#include "Components/SkeletalMeshComponent.h"
#include "Classes/Engine/SkeletalMesh.h"
#include "UObject/ObjectFactory.h"

#include "GameFramework/Character.h"
#include "Math/Rotator.h"
#include "EnemySpawner.h"

#include "Actors/Bullet.h"
#include "Engine/EditorEngine.h"

#include "Animation/AnimStateMachine.h"
#include "Engine/Contents/AnimInstance/LuaScriptAnimInstance.h"

#include "Classes/Components/CapsuleComponent.h"

#include "PhysicsManager.h"
#include "PhysicsEngine/BodyInstance.h"
#include "PhysicsEngine/PhysicsAsset.h"

AEnemy::AEnemy()
    : SkeletalMeshComponent(nullptr)
    , SkeletalMesh(nullptr)
    , FireInterval(4.f)
    , CurrentFireTimer(0.f)
    , bShouldFire(false)
    , Character(nullptr)
    , Direction(FRotator::ZeroRotator)
    , bRagDollCreated(false)
    , bIsAlive(false)
{
}

UObject* AEnemy::Duplicate(UObject* InOuter)
{
    ThisClass* NewActor = Cast<ThisClass>(Super::Duplicate(InOuter));
    NewActor->FireInterval = FireInterval;
    NewActor->CurrentFireTimer = CurrentFireTimer;
    NewActor->bShouldFire = bShouldFire;
    NewActor->SkeletalMeshComponent = SkeletalMeshComponent;
    NewActor->SkeletalMesh = SkeletalMesh;
    NewActor->Character = Character;
    NewActor->Direction = Direction;
    NewActor->bRagDollCreated = bRagDollCreated;
    NewActor->bIsAlive = bIsAlive;

    return NewActor;
}

void AEnemy::Tick(float DeltaTime)
{
    FVector PlayerLocation = GEngine->ActiveWorld->GetMainPlayer()->GetActorLocation();
    Direction = FRotator::MakeLookAtRotation(this->GetActorLocation(), PlayerLocation);
    
    SetActorRotation(FRotator(0, Direction.Yaw, 0));

    // Destroy로직은 다른 곳에 추가할 예정
    //if (CurrentFireTimer >= 2.f && !bRagDollCreated)
    //{
    //    //Destroy();
    //    SkeletalMeshComponent->CreatePhysXGameObject();
    //    SkeletalMeshComponent->bSimulate = true;
    //    SkeletalMeshComponent->bApplyGravity = true;

    //    bRagDollCreated = true;
    //}
        
    CalculateTimer(DeltaTime);
    if (!bShouldFire) return;

    Fire();
}

void AEnemy::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(ELogLevel::Display, TEXT("AEnemy has been spawned."));

    SkeletalMeshComponent = AddComponent<USkeletalMeshComponent>();
    SetRootComponent(SkeletalMeshComponent);

    SkeletalMesh = UAssetManager::Get().GetSkeletalMesh(FName("Contents/Enemy/Pistol_Idle"));
    SkeletalMeshComponent->SetSkeletalMeshAsset(SkeletalMesh);
    SkeletalMeshComponent->StateMachineFileName = "LuaScripts/Animations/EnemyStateMachine.lua";
    Cast<ULuaScriptAnimInstance>(SkeletalMeshComponent->GetAnimInstance())->GetStateMachine()->SetLuaScriptName(SkeletalMeshComponent->StateMachineFileName);
    Cast<ULuaScriptAnimInstance>(SkeletalMeshComponent->GetAnimInstance())->GetStateMachine()->InitLuaStateMachine();

    SetActorLocation(GetOwner()->GetRootComponent()->GetRelativeLocation());
    GetActorRotation();

    bIsAlive = true;
    CurrentFireTimer = 0.0f;
    SetRandomFireInterval();

    // RagDoll을 미리 만듦
    SkeletalMeshComponent->bSimulate = false;
    SkeletalMeshComponent->bApplyGravity = false;

    CreateCollisionCapsule();
}

void AEnemy::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

bool AEnemy::Destroy()
{
    return Super::Destroy();
}

void AEnemy::Destroyed()
{
    Super::Destroyed();

    // 여기다가 파티클 넣으면 될 것 같기도 하고
}

void AEnemy::SetRandomFireInterval()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> distrib(1.0f, 10.0f);
    FireInterval = distrib(gen);
}

void AEnemy::CalculateTimer(float DeltaTime)
{
    if (!bIsAlive)
    {
        bShouldFire = false;
        return;
    }

    CurrentFireTimer += DeltaTime;

    if (CurrentFireTimer >= FireInterval)
    {
        CurrentFireTimer = 0.f;

        bShouldFire = true;
    }
}

void AEnemy::CreateCollisionCapsule()
{
    BodySetup = FObjectFactory::ConstructObject<UBodySetup>(this);

    BodyInstance = new FBodyInstance(SkeletalMeshComponent);
    BodyInstance->bSimulatePhysics = true;
    BodyInstance->bEnableGravity = false;

    FVector Location = GetActorLocation();
    PxVec3 PPos = PxVec3(Location.X, Location.Y, Location.Z);

    //FQuat Quat = GetActorRotation().Quaternion();
    FQuat Quat = FRotator(0.f, 90.f, 90.f).Quaternion();
    PxQuat PQuat = PxQuat(Quat.X, Quat.Y, Quat.Z, Quat.W);

    AggregateGeomAttributes DefaultAttribute; 
    DefaultAttribute.GeomType = EGeomType::ECapsule;
    DefaultAttribute.Offset = FVector::ZeroVector;
    DefaultAttribute.Extent = FVector(80.f, 80.f, 200.f) / 2 * GetActorScale();
    //DefaultAttribute.Rotation = GetActorRotation();
    DefaultAttribute.Rotation = FRotator(0.f, 90.f, 90.f);

    PxVec3 Offset = PxVec3(DefaultAttribute.Offset.X, DefaultAttribute.Offset.Y, DefaultAttribute.Offset.Z);
    FQuat GeomQuat = DefaultAttribute.Rotation.Quaternion();
    PxQuat GeomPQuat = PxQuat(GeomQuat.X, GeomQuat.Y, GeomQuat.Z, GeomQuat.W);
    PxVec3 Extent = PxVec3(DefaultAttribute.Extent.X, DefaultAttribute.Extent.Y, DefaultAttribute.Extent.Z);

    PxShape* PxCapsule = GEngine->PhysicsManager->CreateCapsuleShape(Offset, GeomPQuat, Extent.x, Extent.z);
    BodySetup->AggGeom.CapsuleElems.Add(PxCapsule);

    GameObject* Obj = GEngine->PhysicsManager->CreateGameObject(PPos, PQuat, BodyInstance, BodySetup, ERigidBodyType::DYNAMIC);
}

void AEnemy::Fire()
{
    UWorld* World = GEngine->ActiveWorld;

    ABullet* Bullet = World->SpawnActor<ABullet>();
    Bullet->SetActorLabel(TEXT("OBJ_BULLET"));
    Bullet->SetOwner(this);

    bShouldFire = false;
}
