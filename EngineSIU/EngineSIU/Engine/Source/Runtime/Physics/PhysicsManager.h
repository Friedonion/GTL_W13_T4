#pragma once

#include "Core/HAL/PlatformType.h" // TCHAR 재정의 문제때문에 다른 헤더들보다 앞에 있어야 함

#include <PxPhysicsAPI.h>
#include <DirectXMath.h>
#include <pvd/PxPvd.h>

#include "Container/Array.h"
#include "Container/Map.h"
#include "PhysicsEngine/ConstraintInstance.h"
#include "PhysicsEngine/PhysicsAsset.h"

#include "Delegates/Delegate.h"
#include "Delegates/DelegateCombination.h"

enum class ERigidBodyType : uint8;
struct FBodyInstance;
class UBodySetup;
class UWorld;

using namespace physx;
using namespace DirectX;

class UPrimitiveComponent;
class FSimulationEventCallback;

enum class ECollisionPart : uint8
{
    None,
    Head,
    Body,
    Leg,
};

struct GameObject;
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnGameObjectHit, GameObject* /*HitGameObject*/, AActor* /*HitActor*/, AActor* /*OtherActor*/);

// 게임 오브젝트
struct GameObject {
    PxRigidDynamic* DynamicRigidBody = nullptr;
    PxRigidStatic* StaticRigidBody = nullptr;
    XMMATRIX WorldMatrix = XMMatrixIdentity();

    AActor* OwnerActor = nullptr;
    ECollisionPart PartIdentifier = ECollisionPart::None; // 이 GameObject가 나타내는 신체 부위
    FOnGameObjectHit OnHit;

    void UpdateFromPhysics(PxScene* Scene) {
        PxSceneReadLock scopedReadLock(*Scene);
        PxTransform t = DynamicRigidBody->getGlobalPose();
        PxMat44 mat(t);
        WorldMatrix = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&mat));
    }

    void UpdateForPhysicsScene(PxScene* Scene);

    void SetRigidBodyType(ERigidBodyType RigidBody) const;

    //OnHit
};

class FPhysicsManager
{
public:
    FPhysicsManager();
    ~FPhysicsManager();

    void InitPhysX();
    
    PxScene* CreateScene(UWorld* World);
    PxScene* GetScene(UWorld* World) { return SceneMap[World]; }
    bool ConnectPVD();
    void RemoveScene(UWorld* World) { SceneMap.Remove(World); }
    void SetCurrentScene(UWorld* World) { CurrentScene = SceneMap[World]; }
    void SetCurrentScene(PxScene* Scene) { CurrentScene = Scene; }
    void SetGravity(UWorld* World, FVector Gravity);
    FVector GetGravity(UWorld* World);

    void AddImpulseAtLocation(GameObject* TargetObject, const PxVec3& Impulse, const PxVec3& Location, bool bWakeUp = true) const;
    void AddImpulse(GameObject* TargetObject, const PxVec3& Impulse, bool bWakeUp = true) const;

    void DestroyGameObject(GameObject* GameObject) const;
    
    GameObject CreateBox(const PxVec3& Pos, const PxVec3& HalfExtents) const;
    GameObject* CreateGameObject(AActor* InOwnerActor, const PxVec3& Pos, const PxQuat& Rot, FBodyInstance* BodyInstance, UBodySetup* BodySetup, ERigidBodyType RigidBodyType) const;
    void CreateJoint(const GameObject* Obj1, const GameObject* Obj2, FConstraintInstance* ConstraintInstance, const FConstraintSetup* ConstraintSetup) const;

    PxShape* CreateBoxShape(const PxVec3& Pos, const PxQuat& Quat, const PxVec3& HalfExtents) const;
    PxShape* CreateSphereShape(const PxVec3& Pos, const PxQuat& Quat, float Radius) const;
    PxShape* CreateCapsuleShape(const PxVec3& Pos, const PxQuat& Quat, float Radius, float HalfHeight) const;
    //PxQuat EulerToQuat(const PxVec3& EulerAngles) const;

    PxPhysics* GetPhysics() { return Physics; }
    PxMaterial* GetMaterial() const { return Material; }
    
    void Simulate(float DeltaTime);
    void ShutdownPhysX();
    void CleanupPVD();
    void CleanupScene();

private:
    PxDefaultAllocator Allocator;
    PxDefaultErrorCallback ErrorCallback;
    PxFoundation* Foundation = nullptr;
    PxPhysics* Physics = nullptr;
    TMap<UWorld*, PxScene*> SceneMap;
    PxScene* CurrentScene = nullptr;
    PxMaterial* Material = nullptr;
    PxDefaultCpuDispatcher* Dispatcher = nullptr;
    // 디버깅용
    PxPvd* Pvd = nullptr;
    PxPvdTransport* Transport = nullptr;

    FSimulationEventCallback* SimulationEventCallback;

    PxRigidDynamic* CreateDynamicRigidBody(GameObject* InOwningGameObject, const PxVec3& Pos, const PxQuat& Rot, FBodyInstance* BodyInstance, UBodySetup* BodySetup) const;
    PxRigidStatic* CreateStaticRigidBody(const PxVec3& Pos, const PxQuat& Rot, FBodyInstance* BodyInstance, UBodySetup* BodySetups) const;
    void AttachShapesToActor(PxRigidActor* Actor, UBodySetup* BodySetup) const;
    void ApplyMassAndInertiaSettings(PxRigidDynamic* DynamicBody, const FBodyInstance* BodyInstance) const;
    void ApplyBodyInstanceSettings(PxRigidActor* Actor, const FBodyInstance* BodyInstance) const;
    void ApplyLockConstraints(PxRigidDynamic* DynamicBody, const FBodyInstance* BodyInstance) const;
    void ApplyCollisionSettings(const PxRigidActor* Actor, const FBodyInstance* BodyInstance) const;
    void ApplyShapeCollisionSettings(PxShape* Shape, const FBodyInstance* BodyInstance) const;

};

