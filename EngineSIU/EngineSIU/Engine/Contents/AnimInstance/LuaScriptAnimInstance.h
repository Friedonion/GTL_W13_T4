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
    
    template <typename T>
    void BindLua(T* Owner);

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
    float GetPlayRate() const { return PlayRate; }
    void SetPlayRate(float InRate) { PlayRate = InRate; }

    float GetElapsedTime() const { return ElapsedTime; }
    void SetElapsedTime(float InTime) { ElapsedTime = InTime; }

    int32 GetLoopStartFrame() const { return LoopStartFrame; }
    void SetLoopStartFrame(int32 InFrame) { LoopStartFrame = InFrame; }

    int32 GetLoopEndFrame() const { return LoopEndFrame; }
    void SetLoopEndFrame(int32 InFrame) { LoopEndFrame = InFrame; }

    int32 GetCurrentKey() const { return CurrentKey; }
    void SetCurrentKey(int32 InKey) { CurrentKey = InKey; }

    UAnimSequence* GetCurrentAnim() const { return CurrAnim; }
    void SetAnimation(UAnimSequence* NewAnim, float BlendingTime, bool LoopAnim = false, bool ReverseAnim = false);

public:
    UAnimStateMachine* GetStateMachine() const { return StateMachine; }

private:
    float PreviousTime;
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

template<typename T>
inline void ULuaScriptAnimInstance::BindLua(T* Owner)
{
    StateMachine->InitLuaStateMachine(Owner);
}
