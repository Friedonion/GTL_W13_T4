#pragma once
#include "AnimStateMachine.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

class UAnimSequence;
class UAnimationAsset;
class USkeleton;
class USkeletalMeshComponent;
class UAnimStateMachine;
struct FTransform;
struct FPoseContext;

class UAnimInstance : public UObject
{
    DECLARE_CLASS(UAnimInstance, UObject)

public:
    UAnimInstance() = default;

    virtual void InitializeAnimation();

    void UpdateAnimation(float DeltaSeconds, FPoseContext& OutPose);

    virtual void NativeInitializeAnimation();
    
    virtual void NativeUpdateAnimation(float DeltaSeconds, FPoseContext& OutPose);

    USkeletalMeshComponent* GetSkelMeshComponent() const;

    USkeleton* GetCurrentSkeleton() const { return CurrentSkeleton; }
    
private:
    USkeleton* CurrentSkeleton;
    
};
