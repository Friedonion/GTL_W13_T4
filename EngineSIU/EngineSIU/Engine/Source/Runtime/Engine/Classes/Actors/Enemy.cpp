#include "Enemy.h"
#include "Components/SkeletalMeshComponent.h"
#include "Classes/Engine/SkeletalMesh.h"
#include "UObject/ObjectFactory.h"

#include "GameFramework/Character.h"
#include "Math/Rotator.h"
#include "EnemySpawner.h"
#include "Actors/Player/Player.h"
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
    , bIsAlive(false)
    , BodyInstances()
    , BodySetups()
    , CollisionRigidBodies()
    , bRagDollCreated(false)
{
}

AEnemy::~AEnemy()
{
    for (FConstraintSetup* Setup : ConstraintSetups)
    {
        delete Setup;
    }
    ConstraintSetups.Empty();

    for (FConstraintInstance* Instance : ConstraintInstances)
    {
        delete Instance;
    }
    ConstraintInstances.Empty();

    FTimerManager* TM = GEngine->TimerManager;
    if (TM)
    {
        if (DestroyDelayTimerHandle.IsValid())
        {
            TM->ClearTimer(DestroyDelayTimerHandle);
        }
        if (AttackCheckTimerHandle.IsValid())
        {
            TM->ClearTimer(AttackCheckTimerHandle);
        }
    }
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
    NewActor->bIsAlive = bIsAlive;
    NewActor->BodyInstances = BodyInstances;
    NewActor->BodySetups = BodySetups;
    NewActor->CollisionRigidBodies = CollisionRigidBodies;
    NewActor->bRagDollCreated = bRagDollCreated;

    return NewActor;
}

void AEnemy::Tick(float DeltaTime)
{
    if (bIsAlive)
    {
        FVector PlayerLocation = GEngine->ActiveWorld->GetMainPlayer()->GetActorLocation();
        Direction = FRotator::MakeLookAtRotation(this->GetActorLocation(), PlayerLocation);

        SetActorRotation(FRotator(0, Direction.Yaw, 0));

        CalculateTimer(DeltaTime);
        if (!bShouldFire) return;

        Fire();
    }

    if (bRagDollCreated)
    {
        // 기존 콜리전들 삭제하고
        DestroyCollisions();

        for (auto Body : SkeletalMeshComponent->GetBodies())
        {
            Body->BIGameObject->SetRigidBodyType(ERigidBodyType::DYNAMIC);
        }
    }
}

void AEnemy::BeginPlay()
{
    SkeletalMeshComponent = AddComponent<USkeletalMeshComponent>();
    SetRootComponent(SkeletalMeshComponent);
    Super::BeginPlay();
    UE_LOG(ELogLevel::Display, TEXT("AEnemy has been spawned."));

    SetLuaToPlayAnim();

    SetActorLocation(GetOwner()->GetRootComponent()->GetRelativeLocation());
    GetActorRotation();

    bIsAlive = true;
    bRagDollCreated = false;
    CurrentFireTimer = 0.0f;
    SetRandomFireInterval();

    // RagDoll을 미리 만듦
    SkeletalMeshComponent->bSimulate = false;
    SkeletalMeshComponent->bApplyGravity = false;

    CreateCollisionShapes();
}

void AEnemy::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    //DestroyCollisions(); // 사실상 의미 없음

    Super::EndPlay(EndPlayReason);
}

bool AEnemy::Destroy()
{
    return Super::Destroy();
}

void AEnemy::Destroyed()
{
    Super::Destroyed();
}

void AEnemy::SetRandomFireInterval()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> distrib(1.0f, 3.0f);
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
    SkeletalMeshComponent->SetAnimClass(UClass::FindClass(FName("ULuaScriptAnimInstance")));
    SkeletalMeshComponent->SetAnimationMode(EAnimationMode::AnimationBlueprint);
    if (ULuaScriptAnimInstance* AnimInstance = SkeletalMeshComponent->GetLuaScriptAnimInstance())
    {
        AnimInstance->GetStateMachine()->SetLuaScriptName(SkeletalMeshComponent->StateMachineFileName);
        AnimInstance->GetStateMachine()->InitLuaStateMachine(this);
    }
}

void AEnemy::CreateCollisionShapes()
{
    FName HEAD = TEXT("Head");
    FName BODY = TEXT("Body");
    FName LEG = TEXT("Leg");

    FVector LegSize = FVector(40.f, 40.f, 70.f); 
    FVector BodySize = FVector(40.f, 40.f, 70.f);
    FVector HeadSize = FVector(20.f, 20.f, 30.f); 

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
}

void AEnemy::CreateCollisionBox_Body_Internal(float InCenterZOffsetFromActorBase, FVector InFullSize, FName& BoneName)
{
    // Begin Body
    UBodySetup* BodySetup = FObjectFactory::ConstructObject<UBodySetup>(this);
    BodySetup->BoneName = BoneName;
    FBodyInstance* BodyInstance = new FBodyInstance(SkeletalMeshComponent); 
    BodyInstance->OwnerActor = this;
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
    if (PxBoxShape)
    {
        FName ShapeName = GetActorLabel() + "_" + BoneName.ToString();
        PxBoxShape->setName(*ShapeName.ToString());
    }

    BodySetup->AggGeom.BoxElems.Add(PxBoxShape);

    BodySetups.Add(BodySetup);
    BodyInstances.Add(BodyInstance); 

    BodyInstance->OwnerActor = this;
    GameObject* NewRigidBodyGameObject = GEngine->PhysicsManager->CreateGameObject(this, PPos, PQuat, BodyInstance, BodySetup, ERigidBodyType::DYNAMIC);

    if (NewRigidBodyGameObject)
    {
        if (BoneName == TEXT("Head")) NewRigidBodyGameObject->PartIdentifier = ECollisionPart::Head;
        else if (BoneName == TEXT("Body")) NewRigidBodyGameObject->PartIdentifier = ECollisionPart::Body;
        else if (BoneName == TEXT("Leg")) NewRigidBodyGameObject->PartIdentifier = ECollisionPart::Leg;
        else NewRigidBodyGameObject->PartIdentifier = ECollisionPart::None;

        NewRigidBodyGameObject->OnHit.AddUObject(this, &AEnemy::HandleCollision);

        CollisionRigidBodies.Add(NewRigidBodyGameObject);
    }
}

void AEnemy::CreateCollisionConstraint_Internal(const TArray<UBodySetup*>& InBodySetups)
{
    // Begin Constraint

    for (int Index = 0; Index < BodyInstances.Num() - 1; ++Index)
    {
        if (Index + 1 >= BodyInstances.Num() || !BodyInstances[Index] || !BodyInstances[Index + 1])
        {
            continue;
        }

        if (Index + 1 >= BodySetups.Num() || !BodySetups[Index] || !BodySetups[Index + 1])
        {
            continue;
        }

        FBodyInstance* BodyInstance1 = BodyInstances[Index];
        FBodyInstance* BodyInstance2 = BodyInstances[Index + 1];

        FConstraintSetup* NewConstraintSetup = new FConstraintSetup();
        FConstraintInstance* NewConstraintInstance = new FConstraintInstance();

        FString BoneName1Str = GetCleanBoneName(BodySetups[Index]->BoneName.ToString());
        FString BoneName2Str = GetCleanBoneName(BodySetups[Index + 1]->BoneName.ToString());

        NewConstraintSetup->JointName = FName(*(BoneName1Str + " : " + BoneName2Str)).ToString();
        NewConstraintSetup->ConstraintBone1 = BodySetups[Index]->BoneName.ToString();
        NewConstraintSetup->ConstraintBone2 = BodySetups[Index + 1]->BoneName.ToString();

        ConstraintSetups.Add(NewConstraintSetup);
        ConstraintInstances.Add(NewConstraintInstance);

        GEngine->PhysicsManager->CreateJoint(
            BodyInstance1->BIGameObject,
            BodyInstance2->BIGameObject,
            NewConstraintInstance,
            NewConstraintSetup
        );
    }
    // End Constraint
}

void AEnemy::DestroyCollisions()
{
    for (auto Collision : CollisionRigidBodies)
    {
        GEngine->PhysicsManager->DestroyGameObject(Collision);
    }
    CollisionRigidBodies.Empty();
}

void AEnemy::Die()
{
    if (!bIsAlive) return;

    bIsAlive = false;
    bRagDollCreated = true;
    // Ragdoll 생성
    SkeletalMeshComponent->RigidBodyType = ERigidBodyType::KINEMATIC;
    SkeletalMeshComponent->CreatePhysXGameObject();

    SkeletalMeshComponent->bSimulate = true;
    SkeletalMeshComponent->bApplyGravity = true;

    // TO-DO: 파티클 추가 위치

    FTimerManager* TM = GEngine->TimerManager;
    if (TM)
    {
        if (DestroyDelayTimerHandle.IsValid())
        {
            TM->ClearTimer(DestroyDelayTimerHandle);
        }
        DestroyDelayTimerHandle = TM->SetTimer(this, &AEnemy::DelayedDestroy, 6.0f, false);
    }
}

void AEnemy::Fire()
{
    UWorld* World = GEngine->ActiveWorld;

    ABullet* Bullet = World->SpawnActor<ABullet>();
    Bullet->SetActorLabel(TEXT("OBJ_BULLET"));
    Bullet->SetOwner(this);

    Bullet->SetActorLocation(this->GetActorLocation() + MuzzleOffset +
        this->GetActorForwardVector() * 30.f);
    Bullet->SetActorRotation(this->GetActorRotation());

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

void AEnemy::DelayedDestroy()
{
    Destroy();
}

void AEnemy::PeriodicAttackCheck()
{
    if (!bIsAlive) // 죽었으면 더 이상 실행 안 함
    {
        FTimerManager* TM = GEngine->TimerManager;
        if (TM && AttackCheckTimerHandle.IsValid())
        {
            TM->ClearTimer(AttackCheckTimerHandle);
        }
        return;
    }
}

GameObject* AEnemy::GetRagdollBodyPartByIndex(int32 BodyIndex)
{
    if (SkeletalMeshComponent &&
        SkeletalMeshComponent->GetSkeletalMeshAsset() &&
        SkeletalMeshComponent->GetSkeletalMeshAsset()->GetPhysicsAsset())
    {
        TArray<FBodyInstance*> Bodies = SkeletalMeshComponent->GetBodies();
        if (Bodies.IsValidIndex(BodyIndex) && Bodies[BodyIndex] != nullptr && Bodies[BodyIndex]->BIGameObject != nullptr)
        {
            return Bodies[BodyIndex]->BIGameObject;
        }
    }
    return nullptr;
}

GameObject* AEnemy::GetRandomLegRagdollBodyPart()
{
    // Leg 인덱스 범위: 8 ~ 17 (총 10개)
    constexpr int32 LegMinIndex = 8;
    constexpr int32 LegMaxIndex = 17;
    if (LegMaxIndex < LegMinIndex) return nullptr;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int32> distrib(LegMinIndex, LegMaxIndex);

    int32 RandomLegIndex = distrib(gen);
    return GetRagdollBodyPartByIndex(RandomLegIndex);
}


void AEnemy::ApplyRagdollImpulse(ECollisionPart HitBodyPartType, const FVector& ImpulseDirection, float ImpulseMagnitude)
{
    if (!SkeletalMeshComponent)
    {
        return;
    }

    GameObject* TargetRagdollBodyPart = nullptr;

    switch (HitBodyPartType)
    {
    case ECollisionPart::Head:
        TargetRagdollBodyPart = GetRagdollBodyPartByIndex(0); // 머리는 인덱스 0으로 가정
        break;
    case ECollisionPart::Body:
        TargetRagdollBodyPart = GetRagdollBodyPartByIndex(20); // 몸통은 인덱스 20으로 가정
        break;
    case ECollisionPart::Leg:
        TargetRagdollBodyPart = GetRandomLegRagdollBodyPart(); // 다리는 8~17 중 랜덤
        break;
    case ECollisionPart::None:
    default:
        return;
    }

    if (TargetRagdollBodyPart && TargetRagdollBodyPart->DynamicRigidBody)
    {
        for (auto BodyInstance : SkeletalMeshComponent->GetBodies())
        {
            if (BodyInstance && BodyInstance->BIGameObject)
            {
                BodyInstance->BIGameObject->SetRigidBodyType(ERigidBodyType::DYNAMIC);
            }
        }

        PxVec3 PxImpulse(ImpulseDirection.X * ImpulseMagnitude,
            ImpulseDirection.Y * ImpulseMagnitude,
            ImpulseDirection.Z * ImpulseMagnitude);

        PxTransform RagdollPartGlobalPose = TargetRagdollBodyPart->DynamicRigidBody->getGlobalPose();
        PxVec3 PxImpulseLocation = RagdollPartGlobalPose.p;

        GEngine->PhysicsManager->AddImpulseAtLocation(TargetRagdollBodyPart, PxImpulse, PxImpulseLocation);
    }
}

void AEnemy::HandleCollision(GameObject* HitGameObject, AActor* SelfActor, AActor* OtherActor)
{
    if (!bIsAlive) // 이미 죽었거나, 죽음 처리 요청 중이면 무시
    {
        return;
    }

    if (SelfActor != this || !HitGameObject)
    {
        return;
    }

    ABullet* HittingBullet = Cast<ABullet>(OtherActor);
    if (HittingBullet)
    {
        Die();

        ECollisionPart InitialHitPart = HitGameObject->PartIdentifier;
        FVector ImpulseDirection = HittingBullet->GetActorForwardVector();
        float ImpulseMagnitude = 20000.0f; // 기본 임펄스 크기
        ApplyRagdollImpulse(InitialHitPart, ImpulseDirection, ImpulseMagnitude);

        // HittingBullet->Destroy();
    }
}
