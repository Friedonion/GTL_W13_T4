#include "SimulationEventCallback.h"
#include "GameFramework/Actor.h"
#include "PhysicsEngine/BodyInstance.h"
#include "Components/PrimitiveComponent.h"
#include "Physics/PhysicsManager.h"

void FSimulationEventCallback::onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs)
{
    if (pairHeader.flags & (physx::PxContactPairHeaderFlag::eREMOVED_ACTOR_0 | physx::PxContactPairHeaderFlag::eREMOVED_ACTOR_1))
    {
        return;
    }

    physx::PxRigidActor* pxActor0 = pairHeader.actors[0];
    physx::PxRigidActor* pxActor1 = pairHeader.actors[1];

    if (!pxActor0 || !pxActor1) return;

    GameObject* gameObject0 = static_cast<GameObject*>(pxActor0->userData);
    GameObject* gameObject1 = static_cast<GameObject*>(pxActor1->userData);

    if (!gameObject0 || !gameObject1) return;

    AActor* ownerActor0 = gameObject0->OwnerActor;
    AActor* ownerActor1 = gameObject1->OwnerActor;

    if (!ownerActor0 || !ownerActor1) return;

    if (ownerActor0 == ownerActor1)
    {
        // 필요한 경우 로그 남기고 return
        return;
    }

    // 4. 각 GameObject의 OnHit 델리게이트 브로드캐스트 (변경된 시그니처 사용)
    //    첫 번째 인자: 충돌이 발생한 GameObject 자신
    //    두 번째 인자: 해당 GameObject의 소유자 AActor
    //    세 번째 인자: 충돌 상대방 AActor
    gameObject0->OnHit.Broadcast(gameObject0, ownerActor0, ownerActor1);
    gameObject1->OnHit.Broadcast(gameObject1, ownerActor1, ownerActor0);
}

void FSimulationEventCallback::onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count)
{
}

void FSimulationEventCallback::onWake(physx::PxActor** actors, physx::PxU32 count)
{
}

void FSimulationEventCallback::onSleep(physx::PxActor** actors, physx::PxU32 count)
{
}

void FSimulationEventCallback::onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count)
{
}

void FSimulationEventCallback::onAdvance(const physx::PxRigidBody* const* bodyBuffer, const physx::PxTransform* poseBuffer, const physx::PxU32 count)
{
}
