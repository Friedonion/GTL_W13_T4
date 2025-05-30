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

    void InitializeAnimation();

    void UpdateAnimation(float DeltaSeconds, FPoseContext& OutPose);

    virtual void NativeInitializeAnimation();
    
    virtual void NativeUpdateAnimation(float DeltaSeconds, FPoseContext& OutPose);

    USkeletalMeshComponent* GetSkelMeshComponent() const;

    void TriggerAnimNotifies(float DeltaSeconds);

    USkeleton* GetCurrentSkeleton() const { return CurrentSkeleton; }
    
    virtual void SetAnimState(EAnimState InAnimState);

    virtual UAnimStateMachine* GetStateMachine() const;

    virtual UAnimSequence* GetCurrAnim() const;

    virtual UAnimSequence* GetPrevAnim() const;

    virtual float GetBlendDuration() const;

    virtual void SetBlendDuration(float InBlendDuration);
private:
    USkeleton* CurrentSkeleton = nullptr;
    
};
