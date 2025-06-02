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

    physx::PxRigidActor* actor0 = pairHeader.actors[0];
    physx::PxRigidActor* actor1 = pairHeader.actors[1];

    if (!actor0 || !actor1)
    {
        return;
    }

    FBodyInstance* bodyInstance0 = static_cast<FBodyInstance*>(actor0->userData);
    FBodyInstance* bodyInstance1 = static_cast<FBodyInstance*>(actor1->userData);

    if (!bodyInstance0 || !bodyInstance1)
    {
        return;
    }

    AActor* gameActor0 = nullptr;
    AActor* gameActor1 = nullptr;

    if (bodyInstance0->OwnerComponent)
    {
        gameActor0 = bodyInstance0->OwnerComponent->GetOwner();
    }

    if (bodyInstance1->OwnerComponent)
    {
        gameActor1 = bodyInstance1->OwnerComponent->GetOwner();
    }

    if (!gameActor0 || !gameActor1)
    {
        return;
    }

    if (gameActor0 == gameActor1)
    {
        return;
    }

    if (bodyInstance0->BIGameObject && bodyInstance0->OwnerComponent->GetOwner() == gameActor0 &&
        bodyInstance1->BIGameObject && bodyInstance1->OwnerComponent->GetOwner() == gameActor1)
    {
        bodyInstance0->BIGameObject->OnHit.Broadcast(gameActor0, gameActor1);

        bodyInstance1->BIGameObject->OnHit.Broadcast(gameActor1, gameActor0);
    }
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
