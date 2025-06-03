#include "CapsuleComponent.h"
#include "UObject/Casts.h"
#include "PhysicsManager.h"
#include "Engine/Engine.h"

UCapsuleComponent::UCapsuleComponent()
{
    ShapeType = EShapeType::Capsule;
}

UObject* UCapsuleComponent::Duplicate(UObject* InOuter)
{
    ThisClass* NewComponent = Cast<ThisClass>(Super::Duplicate(InOuter));

    NewComponent->CapsuleHalfHeight = CapsuleHalfHeight;
    NewComponent->CapsuleRadius = CapsuleRadius;
    
    return NewComponent;
}

void UCapsuleComponent::BeginPlay()
{
    Super::BeginPlay();
    if (bSimulate)
    {
        CreatePhysXGameObject();
    }
}

void UCapsuleComponent::SetProperties(const TMap<FString, FString>& InProperties)
{
    Super::SetProperties(InProperties);
    const FString* TempStr = nullptr;
    TempStr = InProperties.Find(TEXT("CapsuleHalfHeight"));
    if (TempStr)
    {
        CapsuleHalfHeight = FCString::Atof(**TempStr);
    }
    TempStr = InProperties.Find(TEXT("CapsuleRadius"));
    if (TempStr)
    {
        CapsuleRadius = FCString::Atof(**TempStr);
    }
}

void UCapsuleComponent::GetProperties(TMap<FString, FString>& OutProperties) const
{
    Super::GetProperties(OutProperties);
    OutProperties.Add(TEXT("CapsuleHalfHeight"), FString::SanitizeFloat(CapsuleHalfHeight));
    OutProperties.Add(TEXT("CapsuleRadius"), FString::SanitizeFloat(CapsuleRadius));
}

void UCapsuleComponent::GetEndPoints(FVector& OutStart, FVector& OutEnd) const
{
    const float LineHalfLength = CapsuleHalfHeight - CapsuleRadius;
    OutStart = GetComponentLocation() + GetUpVector() * LineHalfLength;
    OutEnd = GetComponentLocation() - GetUpVector() * LineHalfLength;
}

void UCapsuleComponent::CreatePhysXGameObject()
{
    if (!bSimulate)
    {
        return;
    }

    BodyInstance = new FBodyInstance(this);

    BodyInstance->bSimulatePhysics = bSimulate;
    BodyInstance->bEnableGravity = bApplyGravity;

    FVector Location = GetComponentLocation();
    PxVec3 PPos = PxVec3(Location.X, Location.Y, Location.Z);

    FQuat Quat = GetComponentRotation().Quaternion();
    PxQuat PQuat = PxQuat(Quat.X, Quat.Y, Quat.Z, Quat.W);

    if (GeomAttributes.Num() == 0)
    {
        AggregateGeomAttributes DefaultAttribute;
        DefaultAttribute.GeomType = EGeomType::ECapsule;
        DefaultAttribute.Extent.X = CapsuleRadius;
        DefaultAttribute.Extent.Z = CapsuleHalfHeight;
        GeomAttributes.Add(DefaultAttribute);
    }

    for (const auto& GeomAttribute : GeomAttributes)
    {
        PxVec3 Offset = PxVec3(GeomAttribute.Offset.X, GeomAttribute.Offset.Y, GeomAttribute.Offset.Z);
        FQuat GeomQuat = GeomAttribute.Rotation.Quaternion();
        PxQuat GeomPQuat = PxQuat(GeomQuat.X, GeomQuat.Y, GeomQuat.Z, GeomQuat.W);
        PxVec3 Extent = PxVec3(GeomAttribute.Extent.X, GeomAttribute.Extent.Y, GeomAttribute.Extent.Z);

        switch (GeomAttribute.GeomType)
        {
        case EGeomType::ESphere:
        {
            PxShape* PxSphere = GEngine->PhysicsManager->CreateSphereShape(Offset, GeomPQuat, Extent.x);
            BodySetup->AggGeom.SphereElems.Add(PxSphere);
            break;
        }
        case EGeomType::EBox:
        {
            PxShape* PxBox = GEngine->PhysicsManager->CreateBoxShape(Offset, GeomPQuat, Extent);
            BodySetup->AggGeom.BoxElems.Add(PxBox);
            break;
        }
        case EGeomType::ECapsule:
        {
            PxShape* PxCapsule = GEngine->PhysicsManager->CreateCapsuleShape(Offset, GeomPQuat, Extent.x, Extent.z);
            // 캡슐이 눕혀있는거 수정
             PxTransform Offset;
            Offset.p = PxVec3(0, 0, 0);
            Offset.q = PxQuat(PxPi / 2, PxVec3(0, 1, 0)); // Y축으로 90도 회전
            PxCapsule->setLocalPose(Offset);
            BodySetup->AggGeom.SphereElems.Add(PxCapsule);
            break;
        }
        }
    }

    GameObject* Obj = GEngine->PhysicsManager->CreateGameObject(this->GetOwner(), PPos, PQuat, BodyInstance, BodySetup, RigidBodyType);

	if (bLockZAxis)
	{
		// Z축 고정 설정
		PxRigidDynamic* DynamicBody = Obj->DynamicRigidBody;
		if (DynamicBody)
		{
			//DynamicBody->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_LINEAR_Z, true);
			DynamicBody->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z, true);
		}
	}
	BodyInstance->BIGameObject = Obj;
}
