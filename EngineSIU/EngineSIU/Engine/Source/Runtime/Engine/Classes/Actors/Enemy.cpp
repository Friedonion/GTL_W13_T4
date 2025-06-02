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
    , bCollisionShapesCreated(false)
    , bRagDollCreated(false)
    , bIsAlive(false)
    , BodyInstances()
    , BodySetups()
    , CollisionRigidBodies()
{
}

UObject* AEnemy::Duplicate(UObject* InOuter)
{
    ThisClass* NewActor = Cast<ThisClass>(Super::Duplicate(InOuter));
    NewActor->SkeletalMeshComponent = SkeletalMeshComponent;
    NewActor->SkeletalMesh = SkeletalMesh;
    NewActor->FireInterval = FireInterval;
    NewActor->CurrentFireTimer = CurrentFireTimer;
    NewActor->bShouldFire = bShouldFire;
    NewActor->Character = Character;
    NewActor->Direction = Direction;
    NewActor->bCollisionShapesCreated = bCollisionShapesCreated;
    NewActor->bRagDollCreated = bRagDollCreated;
    NewActor->bIsAlive = bIsAlive;
    NewActor->BodyInstances = BodyInstances;
    NewActor->BodySetups = BodySetups;
    NewActor->CollisionRigidBodies = CollisionRigidBodies;

    return NewActor;
}

void AEnemy::Tick(float DeltaTime)
{
    FVector PlayerLocation = GEngine->ActiveWorld->GetMainPlayer()->GetActorLocation();
    Direction = FRotator::MakeLookAtRotation(this->GetActorLocation(), PlayerLocation);
    
    SetActorRotation(FRotator(0, Direction.Yaw, 0));

    // Destroy로직은 다른 곳에 추가할 예정
    if (CurrentFireTimer >= 30.f && !bRagDollCreated)
    {
        //Destroy();
        //DestroyCollisionCapsule();
     
        SkeletalMeshComponent->CreatePhysXGameObject();
        SkeletalMeshComponent->bSimulate = true;
        SkeletalMeshComponent->bApplyGravity = true;

        bRagDollCreated = true;
    }
        
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

    SetLuaToPlayAnim();

    SetActorLocation(GetOwner()->GetRootComponent()->GetRelativeLocation());
    GetActorRotation();

    bIsAlive = true;
    CurrentFireTimer = 0.0f;
    SetRandomFireInterval();

    // RagDoll을 미리 만듦
    SkeletalMeshComponent->bSimulate = false;
    SkeletalMeshComponent->bApplyGravity = false;

    CreateCollisionShapes();
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

void AEnemy::SetLuaToPlayAnim()
{
    SkeletalMesh = UAssetManager::Get().GetSkeletalMesh(FName("Contents/Enemy/Pistol_Idle"));
    SkeletalMeshComponent->SetSkeletalMeshAsset(SkeletalMesh);
    SkeletalMeshComponent->StateMachineFileName = "LuaScripts/Animations/EnemyStateMachine.lua";
    Cast<ULuaScriptAnimInstance>(SkeletalMeshComponent->GetAnimInstance())->GetStateMachine()->SetLuaScriptName(SkeletalMeshComponent->StateMachineFileName);
    Cast<ULuaScriptAnimInstance>(SkeletalMeshComponent->GetAnimInstance())->GetStateMachine()->InitLuaStateMachine();
}

void AEnemy::CreateCollisionShapes()
{
    FName HEAD = TEXT("HEAD");
    FName BODY = TEXT("BODY");
    FName LEG = TEXT("LEG");

    FVector LegSize = FVector(40.f, 40.f, 90.f); 
    FVector BodySize = FVector(40.f, 40.f, 100.f);
    FVector HeadSize = FVector(30.f, 30.f, 40.f); 

    float currentZOffset = 0.f;
    float legCenterZ = currentZOffset + LegSize.Z / 2.f;
    CreateCollisionBox_Body_Internal(legCenterZ, LegSize, LEG);
    currentZOffset += LegSize.Z;

    float bodyCenterZ = currentZOffset + BodySize.Z / 2.f;
    CreateCollisionBox_Body_Internal(bodyCenterZ, BodySize, BODY);
    currentZOffset += BodySize.Z;

    float headCenterZ = currentZOffset + HeadSize.Z / 2.f;
    CreateCollisionBox_Body_Internal(headCenterZ, HeadSize, HEAD);

    CreateCollisionConstraint_Internal(BodySetups);
    bCollisionShapesCreated = true;
}

void AEnemy::CreateCollisionBox_Body_Internal(float InCenterZOffsetFromActorBase, FVector InFullSize, FName& BoneName)
{
    // Begin Body
    UBodySetup* BodySetup = FObjectFactory::ConstructObject<UBodySetup>(this);
    FBodyInstance* BodyInstance = new FBodyInstance(SkeletalMeshComponent); 
    BodyInstance->bSimulatePhysics = true;
    BodyInstance->bEnableGravity = false;

    FVector BoxWorldCenterLocation = GetActorLocation() + FVector(0, 0, InCenterZOffsetFromActorBase);
    PxVec3 PPos = PxVec3(BoxWorldCenterLocation.X, BoxWorldCenterLocation.Y, BoxWorldCenterLocation.Z);

    FQuat ActorQuat = GetActorRotation().Quaternion();
    PxQuat PQuat = PxQuat(ActorQuat.X, ActorQuat.Y, ActorQuat.Z, ActorQuat.W);

    AggregateGeomAttributes DefaultAttribute;
    DefaultAttribute.GeomType = EGeomType::EBox;
    DefaultAttribute.Offset = FVector::ZeroVector;
    DefaultAttribute.Extent = InFullSize / 2.f * GetActorScale();
    DefaultAttribute.Rotation = FRotator::ZeroRotator;

    PxVec3 PxShapeLocalOffset = PxVec3(DefaultAttribute.Offset.X, DefaultAttribute.Offset.Y, DefaultAttribute.Offset.Z);
    FQuat ShapeLocalGeomQuat = DefaultAttribute.Rotation.Quaternion(); // FRotator::ZeroRotator.Quaternion() -> FQuat::Identity
    PxQuat PxShapeLocalGeomPQuat = PxQuat(ShapeLocalGeomQuat.X, ShapeLocalGeomQuat.Y, ShapeLocalGeomQuat.Z, ShapeLocalGeomQuat.W);
    PxVec3 PxExtent = PxVec3(DefaultAttribute.Extent.X, DefaultAttribute.Extent.Y, DefaultAttribute.Extent.Z);

    PxShape* PxBoxShape = GEngine->PhysicsManager->CreateBoxShape(PxShapeLocalOffset, PxShapeLocalGeomPQuat, PxExtent);
    BodySetup->AggGeom.BoxElems.Add(PxBoxShape);

    BodySetups.Add(BodySetup);
    BodyInstances.Add(BodyInstance);

    CollisionRigidBodies.Add(GEngine->PhysicsManager->CreateGameObject(PPos, PQuat, BodyInstance, BodySetup, ERigidBodyType::DYNAMIC));
}

void AEnemy::CreateCollisionConstraint_Internal(const TArray<UBodySetup*> BodySetups)
{
    // Begin Constraint
    FConstraintInstance* NewConstraintInstance = new FConstraintInstance();
    FConstraintSetup* NewConstraint = new FConstraintSetup();
    FBodyInstance* BodyInstance1 = nullptr;
    FBodyInstance* BodyInstance2 = nullptr;

    for (int Index = 0; Index < BodyInstances.Num()-1; Index++)
    {
        // 0: Leg
        // 1: Body
        // 2: Head
        BodyInstance1 = BodyInstances[Index];
        BodyInstance2 = BodyInstances[Index + 1];

        NewConstraint->JointName = GetCleanBoneName(BodySetups[Index]->BoneName.ToString()) + " : " + GetCleanBoneName(BodySetups[Index + 1]->BoneName.ToString());
        NewConstraint->ConstraintBone1 = BodySetups[Index]->BoneName.ToString();
        NewConstraint->ConstraintBone2 = BodySetups[Index + 1]->BoneName.ToString();

        if (BodyInstance1 && BodyInstance2)
        {
            ConstraintSetups.Add(NewConstraint);
            ConstraintInstances.Add(NewConstraintInstance);

            GEngine->PhysicsManager->CreateJoint(BodyInstance1->BIGameObject, BodyInstance2->BIGameObject, NewConstraintInstance, ConstraintSetups[Index]);

        }

    }
    // End Constraint
}

void AEnemy::DestroyCollisions()
{
    for (auto Collision : CollisionRigidBodies)
    {
        GEngine->PhysicsManager->DestroyGameObject(Collision);
    }
}

void AEnemy::Fire()
{
    UWorld* World = GEngine->ActiveWorld;

    ABullet* Bullet = World->SpawnActor<ABullet>();
    Bullet->SetActorLabel(TEXT("OBJ_BULLET"));
    Bullet->SetOwner(this);

    bShouldFire = false;
}

FString AEnemy::GetCleanBoneName(const FString& InFullName)
{
    // 1) 계층 구분자 '|' 뒤 이름만 취하기
    int32 barIdx = InFullName.FindChar(TEXT('|'),
        /*case*/ ESearchCase::CaseSensitive,
        /*dir*/  ESearchDir::FromEnd);
    FString name = (barIdx != INDEX_NONE)
        ? InFullName.RightChop(barIdx + 1)
        : InFullName;

    // 2) 네임스페이스 구분자 ':' 뒤 이름만 취하기
    int32 colonIdx = name.FindChar(TEXT(':'),
        /*case*/ ESearchCase::CaseSensitive,
        /*dir*/  ESearchDir::FromEnd);
    if (colonIdx != INDEX_NONE)
    {
        return name.RightChop(colonIdx + 1);
    }
    return name;
}
