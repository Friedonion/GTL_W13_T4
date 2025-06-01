#pragma once
#include "ReferenceSkeleton.h"
#include "SceneComponent.h"

class USkeletalMeshComponent;

class USocketComponent : public USceneComponent
{
    DECLARE_CLASS(USocketComponent, USceneComponent)
public:
    USocketComponent();
    virtual ~USocketComponent() override = default;

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime) override;
    virtual UObject* Duplicate(UObject* InOuter) override;

    FReferenceSkeleton& GetRefSkeletal() const;

    FName Socket = "mixamorig:Weapon";
    // USkeletalMeshComponent* SkeletalMeshComponent = nullptr;
};
