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

#include "GameFramework/UncannyGameMode.h"
#include "Components/StaticMeshComponent.h"
#include "Classes/Engine/FObjLoader.h"
#include "Components/SocketComponent.h"

#include "Lua/LuaScriptComponent.h"
#include "Lua/LuaScriptManager.h"
#include "Lua/LuaUtils/LuaTypeMacros.h"

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
    , Weapon(nullptr)
    , StaticMeshComponent(nullptr)
    , SocketComponent()
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
    NewActor->Weapon = Weapon;
    NewActor->SocketComponent = SocketComponent;
    NewActor->StaticMeshComponent = StaticMeshComponent;

    return NewActor;
}

void AEnemy::Tick(float DeltaTime)
{
    if (bIsAlive)
    {
        FVector PlayerLocation = GEngine->ActiveWorld->GetMainPlayer()->GetActorLocation();
        
        FVector Disp = PlayerLocation - GetActorLocation();

        float Distance = Disp.Length();

        if (Distance < 1000)
        {
            State = Attack;
        }
        else
        {
            State = Patrol;
        }

        if (State == Patrol)
        {
            FVector CurrentLocation = GetActorLocation();
            // 다음 위치 계산
            FVector NextLocation = CurrentLocation + MoveDirection * DeltaTime * MoveSpeed;

            // 두 순찰 지점(PatrolA, PatrolB)과의 거리
            float ToA = FVector::Dist(NextLocation, PatrolA);
            float ToB = FVector::Dist(NextLocation, PatrolB);

            // PatrolA 또는 PatrolB에 도달하면 방향 반전
            if (ToA < 100.f) // 오차 허용치(1cm)
            {
                MoveDirection = (PatrolB - PatrolA).GetSafeNormal();
            }
            else if (ToB < 100.f)
            {
                MoveDirection = (PatrolA - PatrolB).GetSafeNormal();
            }

            FRotator Rot = FRotator::MakeLookAtRotation(FVector::ZeroVector, MoveDirection);

            // 위치 이동
            SetActorLocation(NextLocation);
            SetActorRotation(Rot);
        }
        else // Attack
        {


            Direction = FRotator::MakeLookAtRotation(this->GetActorLocation(), PlayerLocation);

            SetActorRotation(FRotator(0, Direction.Yaw, 0));

            CalculateTimer(DeltaTime);

            if (!bShouldFire) return;

            Fire();
        }

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
    LuaScriptComponent->SetScriptName("LuaScripts/Actors/Enemy.lua");
    Super::BeginPlay();

    RegisterLuaType(FLuaScriptManager::Get().GetLua());

    SetLuaToPlayAnim();
    //if (GetOwner())
    //{
    //    SetActorLocation(GetOwner()->GetRootComponent()->GetRelativeLocation());
    //}
    //else
    //{

    if (SkeletalMeshComponent)
    {
        SkeletalMeshComponent->BindAnimScriptInstance(this);
    }

    SetActorLocation(GetOwner()->GetRootComponent()->GetRelativeLocation());
    GetActorRotation();

    bIsAlive = true;
    bRagDollCreated = false;
    CurrentFireTimer = 0.0f;
    SetRandomFireInterval();

    // RagDoll을 미리 만듦
    SkeletalMeshComponent->bSimulate = false;
    SkeletalMeshComponent->bApplyGravity = false;


    Weapon = FObjManager::GetStaticMesh(L"Contents/Glock18/Glock17.obj");

    SocketComponent = AddComponent<USocketComponent>();
    SocketComponent->SetupAttachment(SkeletalMeshComponent);
    SocketComponent->Socket = "mixamorig:RightHandIndex1";

    StaticMeshComponent = AddComponent<UStaticMeshComponent>();
    StaticMeshComponent->SetupAttachment(SocketComponent);
    StaticMeshComponent->SetStaticMesh(Weapon);

    StaticMeshComponent->SetRelativeScale3D(FVector(5.f, 5.f, 5.f));
    StaticMeshComponent->SetRelativeRotation(FRotator(180.f, 0.f, 0.f));

    CreateCollisionShapes();

    MoveDirection = PatrolA - PatrolB;
    MoveDirection.Normalize();

}

void AEnemy::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    //DestroyCollisions(); // 사실상 의미 없음

    Super::EndPlay(EndPlayReason);
}

bool AEnemy::Destroy()
{
    if (SkeletalMeshComponent->BodyInstance)
    {
        GEngine->PhysicsManager->DestroyGameObject(SkeletalMeshComponent->BodyInstance->BIGameObject);

    }

    for (FBodyInstance* BodyInstance : SkeletalMeshComponent->GetBodies())
    {
        if (BodyInstance && BodyInstance->BIGameObject)
        {
            GEngine->PhysicsManager->DestroyGameObject(BodyInstance->BIGameObject);
        }
    }
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
        //AnimInstance->GetStateMachine()->InitLuaStateMachine(this);
        AnimInstance->SetPlaying(true);
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
    BodyInstance->CollisionEnabled = ECollisionEnabled::QueryAndPhysics;  // 물리와 쿼리 모두 활성화
    BodyInstance->bUseCCD = true;                                        // CCD 활성화
    BodyInstance->bStartAwake = true;                                    // 항상 깨어있는 상태로 시작
    BodyInstance->PositionSolverIterationCount = 8;                     // 위치 솔버 반복 횟수 증가
    BodyInstance->VelocitySolverIterationCount = 4;                     // 속도 솔버 반복 횟수 증가

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

    /*        KINEMATICS로 변경           */
    // PxScene 외부에서 위치변경해야함
    NewRigidBodyGameObject->SetRigidBodyType(ERigidBodyType::KINEMATIC);

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

void AEnemy::RegisterLuaType(sol::state& Lua)
{
    DEFINE_LUA_TYPE_WITH_PARENT(AEnemy, (sol::bases<AActor>()),
        "State", &AEnemy::State
    )
}

bool AEnemy::BindSelfLuaProperties()
{
    if (!LuaScriptComponent)
    {
        return false;
    }
    // LuaScript Load 실패.
    if (!LuaScriptComponent->LoadScript())
    {
        return false;
    }

    sol::table& LuaTable = LuaScriptComponent->GetLuaSelfTable();
    if (!LuaTable.valid())
    {
        return false;
    }

    // 자기 자신 등록.
    // self에 this를 하게 되면 내부에서 임의로 Table로 바꿔버리기 때문에 self:함수() 형태의 호출이 불가능.
    // 자기 자신 객체를 따로 넘겨주어야만 AActor:GetName() 같은 함수를 실행시켜줄 수 있다.
    LuaTable["this"] = this;
    LuaTable["Name"] = *GetName(); // FString 해결되기 전까지 임시로 Table로 전달.
    // 이 아래에서 또는 하위 클래스 함수에서 멤버 변수 등록.

    return true;
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
        float ImpulseMagnitude = 200000.0f; // 기본 임펄스 크기
        ApplyRagdollImpulse(InitialHitPart, ImpulseDirection, ImpulseMagnitude);

        if (AUncannyGameMode* GameMode = Cast<AUncannyGameMode>(GetWorld()->GetGameMode()))
        {
            GameMode->AddKill();
            uintptr_t Address = reinterpret_cast<uintptr_t>(OtherActor);
            int Value = (static_cast<int>(Address) % 5) + 1; // 1~5
            int32 BulletCount = GameMode->GetBulletCount();
            GameMode->SetBulletCount(BulletCount + Value);
        }

        // HittingBullet->Destroy();
    }
}
