#pragma once
#include "Animation/Animinstance.h"
#include "Animation/AnimStateMachine.h"
#include "UObject/ObjectMacros.h"

class UAnimSequence;
class UAnimationAsset;

class ULuaScriptAnimInstance : public UAnimInstance
{
    DECLARE_CLASS(ULuaScriptAnimInstance, UAnimInstance)

public:
    ULuaScriptAnimInstance();

    virtual void InitializeAnimation() override;
    virtual void NativeInitializeAnimation() override;

    virtual void NativeUpdateAnimation(float DeltaSeconds, FPoseContext& OutPose) override;
    
    void SetPlaying(bool bIsPlaying)
    {
        bPlaying = bIsPlaying;
    }

    bool IsPlaying() const
    {
        return bPlaying;
    }

    void SetReverse(bool bIsReverse)
    {
        bReverse = bIsReverse;
    }
    
    bool IsReverse() const
    {
        return bReverse;
    }

    void SetLooping(bool bIsLooping)
    {
        bLooping = bIsLooping;
    }

    bool IsLooping() const
    {
        return bLooping;
    }

    void SetAnimation(UAnimSequence* NewAnim, float BlendingTime, float LoopAnim = false, bool ReverseAnim = false);

private:
    float ElapsedTime;
    float PlayRate;
    
    bool bLooping;
    bool bPlaying;
    bool bReverse;

    int32 LoopStartFrame;

    int32 LoopEndFrame;

    int CurrentKey;
    
    UAnimSequence* PrevAnim;
    UAnimSequence* CurrAnim;
    
    float BlendAlpha;
    float BlendStartTime;
    float BlendDuration;
    
    bool bIsBlending;
    
    UPROPERTY(EditAnywhere, UAnimStateMachine*, StateMachine, = nullptr)
    
};
