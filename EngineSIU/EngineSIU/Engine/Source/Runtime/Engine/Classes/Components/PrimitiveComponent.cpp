#include "PrimitiveComponent.h"

#include "BoxComponent.h"
#include "CapsuleComponent.h"
#include "PhysicsManager.h"
#include "SphereComp.h"
#include "SphereComponent.h"
#include "Engine/Engine.h"
#include "UObject/Casts.h"
#include "Engine/OverlapInfo.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Actor.h"
#include "Math/JungleMath.h"
#include "Misc/Parse.h"
#include "World/World.h"
#include "PhysicsEngine/PhysicsAsset.h"

// 언리얼 엔진에서도 여기에서 FOverlapInfo의 생성자를 정의하고 있음.
FOverlapInfo::FOverlapInfo(UPrimitiveComponent* InComponent, int32 InBodyIndex)
    : bFromSweep(false)
{
    if (InComponent)
    {
        OverlapInfo.HitActor = InComponent->GetOwner();
    }
    OverlapInfo.Component = InComponent;
    OverlapInfo.Item = InBodyIndex;
}

// Helper for finding the index of an FOverlapInfo in an Array using the FFastOverlapInfoCompare predicate, knowing that at least one overlap is valid (non-null).
template<class AllocatorType>
int32 IndexOfOverlapFast(const TArray<FOverlapInfo, AllocatorType>& OverlapArray, const FOverlapInfo& SearchItem)
{
    return OverlapArray.IndexOfByPredicate(FFastOverlapInfoCompare(SearchItem));
}

// Version that works with arrays of pointers and pointers to search items.
template<class AllocatorType>
int32 IndexOfOverlapFast(const TArray<const FOverlapInfo*, AllocatorType>& OverlapPtrArray, const FOverlapInfo* SearchItem)
{
    return OverlapPtrArray.IndexOfByPredicate(FFastOverlapInfoCompare(*SearchItem));
}

template <class ElementType, class AllocatorType1, class AllocatorType2>
static void GetPointersToArrayData(TArray<const ElementType*, AllocatorType1>& Pointers, const TArray<ElementType, AllocatorType2>& DataArray)
{
    const int32 NumItems = DataArray.Num();
    Pointers.SetNum(NumItems);
    for (int32 i=0; i < NumItems; i++)
    {
        Pointers[i] = &(DataArray[i]);
    }
}

template <class ElementType, class AllocatorType1>
static void GetPointersToArrayData(TArray<const ElementType*, AllocatorType1>& Pointers, const TArray<const ElementType>& DataArray)
{
    const int32 NumItems = DataArray.Num();
    Pointers.SetNum(NumItems);
    for (int32 i=0; i < NumItems; i++)
    {
        Pointers[i] = &(DataArray[i]);
    }
}

// Helper to initialize an array to point to data members in another array which satisfy a predicate.
template <class ElementType, class AllocatorType1, class AllocatorType2, typename PredicateT>
static void GetPointersToArrayDataByPredicate(TArray<const ElementType*, AllocatorType1>& Pointers, const TArray<ElementType, AllocatorType2>& DataArray, PredicateT Predicate)
{
    Pointers.Reserve(Pointers.Num() + DataArray.Num());
    for (const ElementType& Item : DataArray)
    {
        if (std::invoke(Predicate, Item))
        {
            Pointers.Add(&Item);
        }
    }
}

template <class ElementType, class AllocatorType1, typename PredicateT>
static void GetPointersToArrayDataByPredicate(TArray<const ElementType*, AllocatorType1>& Pointers, const TArray<const ElementType>& DataArray, PredicateT Predicate)
{
    Pointers.Reserve(Pointers.Num() + DataArray.Num());
    for (const ElementType& Item : DataArray)
    {
        if (std::invoke(Predicate, Item))
        {
            Pointers.Add(&Item);
        }
    }
}

bool AreActorsOverlapping(const AActor& A, const AActor& B)
{
    // Due to the implementation of IsOverlappingActor() that scans and queries all owned primitive components and their overlaps,
    // we guess that it's cheaper to scan the shorter of the lists.
    if (A.GetComponents().Num() <= B.GetComponents().Num())
    {
        return A.IsOverlappingActor(&B);
    }
    else
    {
        return B.IsOverlappingActor(&A);
    }
}

static bool CanComponentsGenerateOverlap(const UPrimitiveComponent* MyComponent, /*const*/ UPrimitiveComponent* OtherComp)
{
    return OtherComp
        && OtherComp->bGenerateOverlapEvents
        && MyComponent
        && MyComponent->bGenerateOverlapEvents
        /* && MyComponent->GetCollisionResponseToComponent(OtherComp) == ECR_Overlap */;
}

struct FPredicateFilterCanOverlap
{
    FPredicateFilterCanOverlap(const UPrimitiveComponent& OwningComponent)
        : MyComponent(OwningComponent)
    {
    }

    bool operator() (const FOverlapInfo& Info) const
    {
        return CanComponentsGenerateOverlap(&MyComponent, Info.OverlapInfo.GetComponent());
    }

private:
    const UPrimitiveComponent& MyComponent;
};

static bool ShouldIgnoreOverlapResult(const UWorld* World, const AActor* ThisActor, const UPrimitiveComponent& ThisComponent, const AActor* OtherActor, const UPrimitiveComponent& OtherComponent, bool bCheckOverlapFlags)
{
    // Don't overlap with self
    if (&ThisComponent == &OtherComponent)
    {
        return true;
    }

    if (bCheckOverlapFlags)
    {
        // Both components must set GetGenerateOverlapEvents()
        if (!ThisComponent.GetGenerateOverlapEvents() || !OtherComponent.GetGenerateOverlapEvents())
        {
            return true;
        }
    }

    if (!ThisActor || !OtherActor)
    {
        return true;
    }

    if (!World /* || OtherActor == World->GetWorldSettings() || (OtherActor.GetCachedActor() && !OtherActor.GetCachedActor()->IsActorInitialized()) */)
    {
        return true;
    }

    return false;
}

UPrimitiveComponent::UPrimitiveComponent()
{
    ////////////////////////////////////// 테스트 코드
    BodySetup = FObjectFactory::ConstructObject<UBodySetup>(this);
}

UObject* UPrimitiveComponent::Duplicate(UObject* InOuter)
{
    ThisClass* NewComponent = Cast<ThisClass>(Super::Duplicate(InOuter));

    NewComponent->AABB = AABB;
    NewComponent->bSimulate = bSimulate;
    NewComponent->bApplyGravity = bApplyGravity;
    NewComponent->GeomAttributes = GeomAttributes;
    NewComponent->RigidBodyType = RigidBodyType;

    return NewComponent;
}

void UPrimitiveComponent::InitializeComponent()
{
    Super::InitializeComponent();

    // OnComponentBeginOverlap.AddLambda(
    //     [](UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Hit)
    //     {
    //         UE_LOG(ELogLevel::Display, "Component [%s] Begin overlap with [%s]", *OverlappedComponent->GetName(), *OtherComp->GetName());
    //     }
    // );
    //
    // OnComponentEndOverlap.AddLambda(
    //     [](UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
    //     {
    //         UE_LOG(ELogLevel::Display, "Component [%s] End overlap with [%s]", *OverlappedComponent->GetName(), *OtherComp->GetName());
    //     }
    // );
}

void UPrimitiveComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);
}

void UPrimitiveComponent::EndPhysicsTickComponent(float DeltaTime)
{
    Super::EndPhysicsTickComponent(DeltaTime);
    // Physics simulation
    if (bSimulate && BodyInstance && RigidBodyType != ERigidBodyType::STATIC)
    {
        BodyInstance->BIGameObject->UpdateFromPhysics(GEngine->PhysicsManager->GetScene(GEngine->ActiveWorld));
        XMMATRIX Matrix = BodyInstance->BIGameObject->WorldMatrix;
        
        XMVECTOR scale, rotation, translation;
        XMMatrixDecompose(&scale, &rotation, &translation, Matrix);
        
        // ✅ 위치 추출
        XMFLOAT3 pos;
        XMStoreFloat3(&pos, translation);
        
        // ✅ 쿼터니언에서 오일러 각도로 변환
        XMFLOAT4 quat;
        XMStoreFloat4(&quat, rotation);
        
        FQuat MyQuat = FQuat(quat.x, quat.y, quat.z, quat.w);
        FRotator Rotator = MyQuat.Rotator();
        
        // ✅ Unreal Engine에 적용 (라디안 → 도 변환)
        SetWorldLocation(FVector(pos.x, pos.y, pos.z));
        SetWorldRotation(Rotator);
    }
}

bool UPrimitiveComponent::IntersectRayTriangle(const FVector& RayOrigin, const FVector& RayDirection, const FVector& v0, const FVector& v1, const FVector& v2, float& OutHitDistance) const
{
    const FVector Edge1 = v1 - v0;
    const FVector Edge2 = v2 - v0;
    
    FVector FrayDirection = RayDirection;
    FVector h = FrayDirection.Cross(Edge2);
    float a = Edge1.Dot(h);

    if (fabs(a) < SMALL_NUMBER)
    {
        return false; // Ray와 삼각형이 평행한 경우
    }

    float f = 1.0f / a;
    FVector s = RayOrigin - v0;
    float u = f * s.Dot(h);
    if (u < 0.0f || u > 1.0f)
    {
        return false;
    }

    FVector q = s.Cross(Edge1);
    float v = f * FrayDirection.Dot(q);
    if (v < 0.0f || (u + v) > 1.0f)
    {
        return false;
    }

    float t = f * Edge2.Dot(q);
    if (t > SMALL_NUMBER)
    {
        OutHitDistance = t;
        return true;
    }

    return false;
}

void UPrimitiveComponent::GetProperties(TMap<FString, FString>& OutProperties) const
{
    Super::GetProperties(OutProperties);
    OutProperties.Add(TEXT("m_Type"), m_Type);
    OutProperties.Add(TEXT("AABB_min"), AABB.MinLocation.ToString());
    OutProperties.Add(TEXT("AABB_max"), AABB.MaxLocation.ToString());
    OutProperties.Add(TEXT("bSimulate"), bSimulate ? TEXT("true") : TEXT("false"));
    OutProperties.Add(TEXT("bApplyGravity"), bApplyGravity ? TEXT("true") : TEXT("false"));
    OutProperties.Add(TEXT("RigidBodyType"), FString::FromInt(static_cast<uint8>(RigidBodyType)));
}

void UPrimitiveComponent::SetProperties(const TMap<FString, FString>& InProperties)
{
    Super::SetProperties(InProperties);

    const FString* TempStr = nullptr;

    // --- PrimitiveComponent 고유 속성 복원 ---

    TempStr = InProperties.Find(TEXT("m_Type"));
    if (TempStr)
    {
        this->m_Type = *TempStr; // m_Type이 FString이라고 가정
        // 만약 m_Type이 enum이라면 문자열로부터 enum 값을 파싱하는 로직 필요
    }

    const FString* AABBminStr = InProperties.Find(TEXT("AABB_min"));
    if (AABBminStr)
    {
        AABB.MinLocation.InitFromString(*AABBminStr);
    }


    const FString* AABBmaxStr = InProperties.Find(TEXT("AABB_max"));
    if (AABBmaxStr)
    {
        AABB.MaxLocation.InitFromString(*AABBmaxStr);
    }

    if (InProperties.Contains(TEXT("bSimulate")))
    {
        bSimulate = InProperties[TEXT("bSimulate")] == "true";
    }

    if (InProperties.Contains(TEXT("bApplyGravity")))
    {
        bApplyGravity = InProperties[TEXT("bApplyGravity")] == "true";
    }

    if (InProperties.Contains(TEXT("RigidBodyType")))
    {
        RigidBodyType = static_cast<ERigidBodyType>(FString::ToInt(InProperties[TEXT("RigidBodyType")]));
    }
}

void UPrimitiveComponent::BeginComponentOverlap(const FOverlapInfo& OtherOverlap, bool bDoNotifies)
{
    // If pending kill, we should not generate any new overlaps
    if (!this)
    {
        return;
    }

    const bool bComponentsAlreadyTouching = (IndexOfOverlapFast(OverlappingComponents, OtherOverlap) != INDEX_NONE);
    if (!bComponentsAlreadyTouching)
    {
        UPrimitiveComponent* OtherComp = OtherOverlap.OverlapInfo.Component;
        if (CanComponentsGenerateOverlap(this, OtherComp))
        {
            AActor* const OtherActor = OtherComp->GetOwner();
            AActor* const MyActor = GetOwner();

            const bool bSameActor = (MyActor == OtherActor);
            const bool bNotifyActorTouch = bDoNotifies && !bSameActor && !AreActorsOverlapping(*MyActor, *OtherActor);

            // Perform reflexive touch.
            OverlappingComponents.Add(OtherOverlap);                                                // already verified uniqueness above
            OtherComp->OverlappingComponents.AddUnique(FOverlapInfo(this, INDEX_NONE));    // uniqueness unverified, so addunique
            
            const UWorld* World = GetWorld();
            if (bDoNotifies && (World /* && World->HasBegunPlay() */))
            {
                // first execute component delegates
                if (this)
                {
                    OnComponentBeginOverlap.Broadcast(this, OtherActor, OtherComp, OtherOverlap.GetBodyIndex(), OtherOverlap.bFromSweep, OtherOverlap.OverlapInfo);
                }

                if (OtherComp)
                {
                    // Reverse normals for other component. When it's a sweep, we are the one that moved.
                    OtherComp->OnComponentBeginOverlap.Broadcast(OtherComp, MyActor, this, INDEX_NONE, OtherOverlap.bFromSweep, OtherOverlap.bFromSweep ? FHitResult::GetReversedHit(OtherOverlap.OverlapInfo) : OtherOverlap.OverlapInfo);
                }

                // then execute actor notification if this is a new actor touch
                if (bNotifyActorTouch)
                {
                    // Then level-script delegates
                    if (MyActor)
                    {
                        MyActor->OnActorBeginOverlap.Broadcast(MyActor, OtherActor);
                    }

                    if (OtherActor)
                    {
                        OtherActor->OnActorBeginOverlap.Broadcast(OtherActor, MyActor);
                    }
                }
            }
        }
    }
}

void UPrimitiveComponent::EndComponentOverlap(const FOverlapInfo& OtherOverlap, bool bDoNotifies, bool bSkipNotifySelf)
{
    UPrimitiveComponent* OtherComp = OtherOverlap.OverlapInfo.Component;
    if (OtherComp == nullptr)
    {
        return;
    }

    const int32 OtherOverlapIdx = IndexOfOverlapFast(OtherComp->OverlappingComponents, FOverlapInfo(this, INDEX_NONE));
    if (OtherOverlapIdx != INDEX_NONE)
    {
        OtherComp->OverlappingComponents.RemoveAt(OtherOverlapIdx);
    }

    const int32 OverlapIdx = IndexOfOverlapFast(OverlappingComponents, OtherOverlap);
    if (OverlapIdx != INDEX_NONE)
    {
        OverlappingComponents.RemoveAt(OverlapIdx);

        AActor* const MyActor = GetOwner();
        const UWorld* World = GetWorld();
        if (bDoNotifies && (World /* && World->HasBegunPlay() */))
        {
            AActor* const OtherActor = OtherComp->GetOwner();
            if (OtherActor)
            {
                if (!bSkipNotifySelf && this)
                {
                    OnComponentEndOverlap.Broadcast(this, OtherActor, OtherComp, OtherOverlap.GetBodyIndex());
                }

                if (OtherComp)
                {
                    OtherComp->OnComponentEndOverlap.Broadcast(OtherComp, MyActor, this, INDEX_NONE);
                }

                // if this was the last touch on the other actor by this actor, notify that we've untouched the actor as well
                const bool bSameActor = (MyActor == OtherActor);
                if (MyActor && !bSameActor && !AreActorsOverlapping(*MyActor, *OtherActor))
                {            
                    if (MyActor)
                    {
                        MyActor->OnActorEndOverlap.Broadcast(MyActor, OtherActor);
                    }

                    if (OtherActor)
                    {
                        OtherActor->OnActorEndOverlap.Broadcast(OtherActor, MyActor);
                    }
                }
            }
        }
    }
}

bool UPrimitiveComponent::IsOverlappingComponent(const UPrimitiveComponent* OtherComp) const
{
    for (int32 i=0; i < OverlappingComponents.Num(); ++i)
    {
        if (OverlappingComponents[i].OverlapInfo.Component == OtherComp)
        {
            return true;
        }
    }
    return false;
}

bool UPrimitiveComponent::IsOverlappingComponent(const FOverlapInfo& Overlap) const
{
    return OverlappingComponents.Contains(Overlap);
}

bool UPrimitiveComponent::IsOverlappingActor(const AActor* Other) const
{
    if (Other)
    {
        for (int32 OverlapIdx = 0; OverlapIdx < OverlappingComponents.Num(); ++OverlapIdx)
        {
            // UPrimitiveComponent const* const PrimComp = OverlappingComponents[OverlapIdx].OverlapInfo.Component.Get();
            UPrimitiveComponent const* const PrimComp = OverlappingComponents[OverlapIdx].OverlapInfo.Component;
            if ( PrimComp && (PrimComp->GetOwner() == Other) )
            {
                return true;
            }
        }
    }

    return false;
}

void UPrimitiveComponent::GetOverlappingComponents(TArray<UPrimitiveComponent*>& OutOverlappingComponents) const
{
    if (OverlappingComponents.Num() <= 12)
    {
        // TArray with fewer elements is faster than using a set (and having to allocate it).
        OutOverlappingComponents.Empty();
        OutOverlappingComponents.Reserve(OverlappingComponents.Num());
        for (const FOverlapInfo& OtherOverlap : OverlappingComponents)
        {
            UPrimitiveComponent* const OtherComp = OtherOverlap.OverlapInfo.Component;
            if (OtherComp)
            {
                OutOverlappingComponents.AddUnique(OtherComp);
            }
        }
    }
    else
    {
        // Fill set (unique)
        TSet<UPrimitiveComponent*> OverlapSet;
        GetOverlappingComponents(OverlapSet);
    
        // Copy to array
        OutOverlappingComponents.Empty();
        OutOverlappingComponents.Reserve(OverlappingComponents.Num());
        for (UPrimitiveComponent* OtherOverlap : OverlapSet)
        {
            OutOverlappingComponents.Add(OtherOverlap);
        }
    }
}

void UPrimitiveComponent::GetOverlappingComponents(TSet<UPrimitiveComponent*>& OutOverlappingComponents) const
{
    OutOverlappingComponents.Empty();

    for (const FOverlapInfo& OtherOverlap : OverlappingComponents)
    {
        UPrimitiveComponent* const OtherComp = OtherOverlap.OverlapInfo.Component;
        if (OtherComp)
        {
            OutOverlappingComponents.Add(OtherComp);
        }
    }
}

const TArray<FOverlapInfo>& UPrimitiveComponent::GetOverlapInfos() const
{
    return OverlappingComponents;
}

void UPrimitiveComponent::CreatePhysXGameObject()
{
    if (!bSimulate)
    {
        return;
    }
    
    BodyInstance = new FBodyInstance(this);

    ////////////// 테스트 코드
    BodyInstance->bSimulatePhysics = bSimulate;
    BodyInstance->bEnableGravity = bApplyGravity;
    ////////////////////////
    
    FVector Location = GetComponentLocation();
    PxVec3 PPos = PxVec3(Location.X, Location.Y, Location.Z);
    
    FQuat Quat = GetComponentRotation().Quaternion();
    PxQuat PQuat = PxQuat(Quat.X, Quat.Y, Quat.Z, Quat.W);

    if (GeomAttributes.Num() == 0)
    {
        AggregateGeomAttributes DefaultAttribute;
        DefaultAttribute.GeomType = EGeomType::EBox;
        DefaultAttribute.Offset = FVector(AABB.MaxLocation + AABB.MinLocation) / 2;
        DefaultAttribute.Extent = FVector(AABB.MaxLocation - AABB.MinLocation) / 2 * GetComponentScale3D();
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
            BodySetup->AggGeom.SphereElems.Add(PxCapsule);
            break;
        }
        }
    }
    
    GameObject* Obj = GEngine->PhysicsManager->CreateGameObject(PPos, PQuat, BodyInstance,  BodySetup, RigidBodyType);
}

void UPrimitiveComponent::BeginPlay()
{
    USceneComponent::BeginPlay();
}

void UPrimitiveComponent::UpdateOverlapsImpl(const TArray<FOverlapInfo>* NewPendingOverlaps, bool bDoNotifies, const TArray<const FOverlapInfo>* OverlapsAtEndLocation)
{
    const AActor* const MyActor = GetOwner();
    
    if (GetGenerateOverlapEvents())
    {
        if (MyActor)
        {
            const bool bIgnoreChildren = (MyActor->GetRootComponent() == this);
            FMatrix PrevTransform = GetWorldMatrix();
            
            if (NewPendingOverlaps)
            {
                // Note: BeginComponentOverlap() only triggers overlaps where GetGenerateOverlapEvents() is true on both components.
                const int32 NumNewPendingOverlaps = NewPendingOverlaps->Num();
                for (int32 Idx = 0; Idx < NumNewPendingOverlaps; ++Idx)
                {
                    BeginComponentOverlap( (*NewPendingOverlaps)[Idx], bDoNotifies );
                }
            }

            TArray<FOverlapInfo> OverlapMultiResult;
            TArray<const FOverlapInfo*> NewOverlappingComponentPtrs;

            if (this && GetGenerateOverlapEvents())
            {
                // Might be able to avoid testing for new overlaps at the end location.
                if (OverlapsAtEndLocation != nullptr && PrevTransform.Equals(GetWorldMatrix()))
                {
                    const bool bCheckForInvalid = (NewPendingOverlaps && NewPendingOverlaps->Num() > 0);
                    if (bCheckForInvalid)
                    {
                        // BeginComponentOverlap may have disabled what we thought were valid overlaps at the end (collision response or overlap flags could change).
                        GetPointersToArrayDataByPredicate(NewOverlappingComponentPtrs, *OverlapsAtEndLocation, FPredicateFilterCanOverlap(*this));
                    }
                    else
                    {
                        GetPointersToArrayData(NewOverlappingComponentPtrs, *OverlapsAtEndLocation);
                    }
                }
                else
                {
                    UWorld* const MyWorld = GetWorld();
                    TArray<FOverlapResult> Overlaps;

                    /*
                    // note this will optionally include overlaps with components in the same actor (depending on bIgnoreChildren). 
                    FComponentQueryParams Params(SCENE_QUERY_STAT(UpdateOverlaps), bIgnoreChildren ? MyActor : nullptr);
                    Params.bIgnoreBlocks = true;    //We don't care about blockers since we only route overlap events to real overlaps
                    FCollisionResponseParams ResponseParam;
                    InitSweepCollisionParams(Params, ResponseParam);
                    ComponentOverlapMulti(Overlaps, MyWorld, GetComponentLocation(), GetComponentQuat(), GetCollisionObjectType(), Params);
                    */
                    
                    MyWorld->CheckOverlap(this, Overlaps);
                    
                    for (int32 ResultIdx=0; ResultIdx < Overlaps.Num(); ResultIdx++)
                    {
                        const FOverlapResult& Result = Overlaps[ResultIdx];

                        UPrimitiveComponent* const HitComp = Result.Component;
                        if (HitComp && (HitComp != this) && HitComp->GetGenerateOverlapEvents())
                        {
                            const bool bCheckOverlapFlags = false; // Already checked above
                            if (!ShouldIgnoreOverlapResult(MyWorld, MyActor, *this, Result.Actor, *HitComp, bCheckOverlapFlags))
                            {
                                OverlapMultiResult.Emplace(HitComp, Result.ItemIndex);        // don't need to add unique unless the overlap check can return dupes
                            }
                        }
                    }

                    // Fill pointers to overlap results. We ensure below that OverlapMultiResult stays in scope so these pointers remain valid.
                    GetPointersToArrayData(NewOverlappingComponentPtrs, OverlapMultiResult);
                }
            }

            if (OverlappingComponents.Num() > 0)
            {
                TArray<const FOverlapInfo*> OldOverlappingComponentPtrs;
                if (bIgnoreChildren)
                {
                    GetPointersToArrayDataByPredicate(OldOverlappingComponentPtrs, OverlappingComponents, FPredicateOverlapHasDifferentActor(*MyActor));
                }
                else
                {
                    GetPointersToArrayData(OldOverlappingComponentPtrs, OverlappingComponents);
                }

                // Now we want to compare the old and new overlap lists to determine 
                // what overlaps are in old and not in new (need end overlap notifies), and 
                // what overlaps are in new and not in old (need begin overlap notifies).
                // We do this by removing common entries from both lists, since overlapping status has not changed for them.
                // What is left over will be what has changed.
                for (int32 CompIdx = 0; CompIdx < OldOverlappingComponentPtrs.Num() && NewOverlappingComponentPtrs.Num() > 0; ++CompIdx)
                {
                    const FOverlapInfo* SearchItem = OldOverlappingComponentPtrs[CompIdx];
                    const int32 NewElementIdx = IndexOfOverlapFast(NewOverlappingComponentPtrs, SearchItem);
                    if (NewElementIdx != INDEX_NONE)
                    {
                        NewOverlappingComponentPtrs.RemoveAt(NewElementIdx);
                        OldOverlappingComponentPtrs.RemoveAt(CompIdx);
                        --CompIdx;
                    }
                }

                const int32 NumOldOverlaps = OldOverlappingComponentPtrs.Num();
                if (NumOldOverlaps > 0)
                {
                    // Now we have to make a copy of the overlaps because we can't keep pointers to them, that list is about to be manipulated in EndComponentOverlap().
                    TArray<FOverlapInfo> OldOverlappingComponents;
                    OldOverlappingComponents.SetNum(NumOldOverlaps);
                    for (int32 i=0; i < NumOldOverlaps; i++)
                    {
                        OldOverlappingComponents[i] = *(OldOverlappingComponentPtrs[i]);
                    }

                    // OldOverlappingComponents now contains only previous overlaps that are confirmed to no longer be valid.
                    for (const FOverlapInfo& OtherOverlap : OldOverlappingComponents)
                    {
                        if (OtherOverlap.OverlapInfo.Component)
                        {
                            EndComponentOverlap(OtherOverlap, bDoNotifies, false);
                        }
                        else
                        {
                            const int32 StaleElementIndex = IndexOfOverlapFast(OverlappingComponents, OtherOverlap);
                            if (StaleElementIndex != INDEX_NONE)
                            {
                                OverlappingComponents.RemoveAt(StaleElementIndex);
                            }
                        }
                    }
                }
            }

            // NewOverlappingComponents now contains only new overlaps that didn't exist previously.
            for (const FOverlapInfo* NewOverlap : NewOverlappingComponentPtrs)
            {
                BeginComponentOverlap(*NewOverlap, bDoNotifies);
            }
        }
    }
    else
    {
        ClearComponentOverlaps(bDoNotifies, false);
    }

    TArray<USceneComponent*> AttachedChildren(GetAttachChildren());
    for (USceneComponent* const ChildComp : AttachedChildren)
    {
        if (ChildComp)
        {
            ChildComp->UpdateOverlaps(nullptr, bDoNotifies, nullptr);
        }
    }
}

void UPrimitiveComponent::ClearComponentOverlaps(bool bDoNotifies, bool bSkipNotifySelf)
{
    if (OverlappingComponents.Num() > 0)
    {
        // Make a copy since EndComponentOverlap will remove items from OverlappingComponents.
        const TArray<FOverlapInfo> OverlapsCopy(OverlappingComponents);
        for (const FOverlapInfo& OtherOverlap : OverlapsCopy)
        {
            EndComponentOverlap(OtherOverlap, bDoNotifies, bSkipNotifySelf);
        }
    }
}
