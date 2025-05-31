#pragma once
#include "Components/SceneComponent.h"
#include "Engine/OverlapInfo.h"
#include "PhysicsEngine/BodyInstance.h"
#include "PhysicsEngine/PhysicsAsset.h"

class UBodySetup;
DECLARE_MULTICAST_DELEGATE_FiveParams(
    FComponentHitSignature, UPrimitiveComponent* /* HitComponent */, AActor* /* OtherActor */, UPrimitiveComponent* /* OtherComp */,
    FVector /* NormalImpulse */, const FHitResult& /* Hit */
);
DECLARE_MULTICAST_DELEGATE_SixParams(FComponentBeginOverlapSignature, UPrimitiveComponent* /* OverlappedComponent */, AActor* /* OtherActor */, UPrimitiveComponent* /* OtherComp */, int32 /* OtherBodyIndex */, bool /* bFromSweep */, const FHitResult& /* Hit */);
DECLARE_MULTICAST_DELEGATE_FourParams(FComponentEndOverlapSignature, UPrimitiveComponent* /* OverlappedComponent */, AActor* /* OtherActor */, UPrimitiveComponent* /* OtherComp */, int32 /* OtherBodyIndex */);

struct GameObject;

class UPrimitiveComponent : public USceneComponent
{
    DECLARE_CLASS(UPrimitiveComponent, USceneComponent)

public:
    UPrimitiveComponent();

    virtual UObject* Duplicate(UObject* InOuter) override;

    virtual void InitializeComponent() override;
    virtual void TickComponent(float DeltaTime) override;
    virtual void EndPhysicsTickComponent(float DeltaTime) override;
    
    bool IntersectRayTriangle(
        const FVector& RayOrigin, const FVector& RayDirection,
        const FVector& v0, const FVector& v1, const FVector& v2, float& OutHitDistance
    ) const;

    virtual void GetProperties(TMap<FString, FString>& OutProperties) const override;
    virtual void SetProperties(const TMap<FString, FString>& InProperties) override;
    
    FBoundingBox AABB;

    bool GetGenerateOverlapEvents() const { return bGenerateOverlapEvents; }
    
    bool bGenerateOverlapEvents = true;
    bool bBlockComponent = true;

    FBodyInstance* BodyInstance = nullptr;

    FComponentHitSignature OnComponentHit;

    FComponentBeginOverlapSignature OnComponentBeginOverlap;

    FComponentEndOverlapSignature OnComponentEndOverlap;

    UPROPERTY_WITH_FLAGS(EditAnywhere, bool, bSimulate, = false)
    UPROPERTY_WITH_FLAGS(EditAnywhere, bool, bApplyGravity, = false)
    UPROPERTY_WITH_FLAGS(EditAnywhere, ERigidBodyType, RigidBodyType, = ERigidBodyType::DYNAMIC)

    UPROPERTY_WITH_FLAGS(EditAnywhere, TArray<AggregateGeomAttributes>, GeomAttributes)

    /** 
     * Begin tracking an overlap interaction with the component specified.
     * @param OtherComp - The component of the other actor that this component is now overlapping
     * @param bDoNotifies - True to dispatch appropriate begin/end overlap notifications when these events occur.
     * @see [Overlap Events](https://docs.unrealengine.com/InteractiveExperiences/Physics/Collision/Overview#overlapandgenerateoverlapevents)
     */
    void BeginComponentOverlap(const FOverlapInfo& OtherOverlap, bool bDoNotifies);

    /** 
     * Finish tracking an overlap interaction that is no longer occurring between this component and the component specified. 
     * @param OtherComp The component of the other actor to stop overlapping
     * @param bDoNotifies True to dispatch appropriate begin/end overlap notifications when these events occur.
     * @param bSkipNotifySelf True to skip end overlap notifications to this component's.  Does not affect notifications to OtherComp's actor.
     * @see [Overlap Events](https://docs.unrealengine.com/InteractiveExperiences/Physics/Collision/Overview#overlapandgenerateoverlapevents)
     */
    void EndComponentOverlap(const FOverlapInfo& OtherOverlap, bool bDoNotifies=true, bool bSkipNotifySelf=false);

    /**
     * Check whether this component is overlapping another component.
     * @param OtherComp Component to test this component against.
     * @return Whether this component is overlapping another component.
     */
    bool IsOverlappingComponent(const UPrimitiveComponent* OtherComp) const;

    /** Check whether this component has the specified overlap. */
    bool IsOverlappingComponent(const FOverlapInfo& Overlap) const;

    /**
     * Check whether this component is overlapping any component of the given Actor.
     * @param Other Actor to test this component against.
     * @return Whether this component is overlapping any component of the given Actor.
     */
    bool IsOverlappingActor(const AActor* Other) const;

    /** 
     * Returns a list of actors that this component is overlapping.
     * @param OverlappingActors        [out] Returned list of overlapping actors
     * @param ClassFilter            [optional] If set, only returns actors of this class or subclasses
     */
    //void GetOverlappingActors(TArray<AActor*>& OverlappingActors, TSubclassOf<AActor> ClassFilter=nullptr) const;

    /** 
    * Returns the set of actors that this component is overlapping.
    * @param OverlappingActors        [out] Returned list of overlapping actors
    * @param ClassFilter            [optional] If set, only returns actors of this class or subclasses
    */
    //void GetOverlappingActors(TSet<AActor*>& OverlappingActors, TSubclassOf<AActor> ClassFilter=nullptr) const;

    /** Returns unique list of components this component is overlapping. */
    void GetOverlappingComponents(TArray<UPrimitiveComponent*>& OutOverlappingComponents) const;

    /** Returns unique set of components this component is overlapping. */
    void GetOverlappingComponents(TSet<UPrimitiveComponent*>& OutOverlappingComponents) const;

    /** Returns list of components this component is overlapping. */
    const TArray<FOverlapInfo>& GetOverlapInfos() const;

    virtual void CreatePhysXGameObject();

    virtual void BeginPlay() override;
    
protected:
    TArray<FOverlapInfo> OverlappingComponents;

    UBodySetup* BodySetup;

    virtual void UpdateOverlapsImpl(const TArray<FOverlapInfo>* PendingOverlaps = nullptr, bool bDoNotifies = true, const TArray<const FOverlapInfo>* OverlapsAtEndLocation = nullptr) override;

    void ClearComponentOverlaps(bool bDoNotifies, bool bSkipNotifySelf);

    
private:
    FString m_Type;

public:
    FString GetType() { return m_Type; }

    void SetType(const FString& InType)
    {
        m_Type = InType;
        //staticMesh = FEngineLoop::resourceMgr.GetMesh(m_Type);
    }
    
    FBoundingBox GetBoundingBox() const { return AABB; }
};


struct FPredicateOverlapHasDifferentActor
{
    FPredicateOverlapHasDifferentActor(const AActor& Owner)
        : MyOwnerPtr(&Owner)
    {
    }

    bool operator() (const FOverlapInfo& Info)
    {
        // MyOwnerPtr is always valid, so we don't need the IsValid() checks in the WeakObjectPtr comparison operator.
        return MyOwnerPtr != Info.OverlapInfo.HitActor;
    }

private:
    const AActor* const MyOwnerPtr;
};
