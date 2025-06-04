#include "LuaScriptAnimInstance.h"

#include "Animation/AnimationAsset.h"
#include "Animation/AnimationRuntime.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimTypes.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Misc/FrameTime.h"
#include "Animation/AnimStateMachine.h"
#include "UObject/Casts.h"
#include "UObject/ObjectFactory.h"
#include "GameFramework/Pawn.h"
#include "Animation/AnimData/AnimDataModel.h"
#include "Engine/Engine.h"
#include "World/World.h"

ULuaScriptAnimInstance::ULuaScriptAnimInstance()
    : PrevAnim(nullptr)
    , CurrAnim(nullptr)
    , ElapsedTime(0.f)
    , PlayRate(1.f)
    , bLooping(true)
    , bPlaying(true)
    , bReverse(false)
    , LoopStartFrame(0)
    , LoopEndFrame(0)
    , CurrentKey(0)
    , BlendAlpha(0.f)
    , BlendStartTime(0.f)
    , BlendDuration(0.2f)
    , bIsBlending(false)
{
}

void ULuaScriptAnimInstance::InitializeAnimation()
{
    UAnimInstance::InitializeAnimation();

    StateMachine = FObjectFactory::ConstructObject<UAnimStateMachine>(this);
    StateMachine->Initialize(Cast<USkeletalMeshComponent>(GetOuter()), this);
}

void ULuaScriptAnimInstance::NativeInitializeAnimation()
{

}

void ULuaScriptAnimInstance::NativeUpdateAnimation(float DeltaSeconds, FPoseContext& OutPose)
{
    UAnimInstance::NativeUpdateAnimation(DeltaSeconds, OutPose);

    if (GEngine->ActiveWorld->WorldType == EWorldType::PIE && StateMachine)
    {
        StateMachine->ProcessState();
    }

    USkeletalMeshComponent* SkeletalMeshComp = GetSkelMeshComponent();
    if (!CurrAnim || !SkeletalMeshComp || !SkeletalMeshComp->GetSkeletalMeshAsset() || !bPlaying)
        return;

    //UAnimDataModel* DataModel = CurrAnim->GetDataModel();
    //if (!DataModel)
    //    return;

    //const int32 FrameRate = DataModel->GetFrameRate();
    //if (FrameRate <= 0)
    //    return;

    //const float StartTime = static_cast<float>(LoopStartFrame) / FrameRate;
    //const float EndTime = static_cast<float>(LoopEndFrame) / FrameRate;
    const float Direction = bReverse ? -1.f : 1.f;

    PreviousTime = ElapsedTime;
    const float DeltaTime = DeltaSeconds * PlayRate * Direction;
    ElapsedTime += DeltaTime;

    //// 루프 / 비루프 처리
    //if (bLooping)
    //{
    //    const float Duration = EndTime - StartTime;
    //    float Adjusted = FMath::Fmod(ElapsedTime - StartTime, Duration);
    //    if (Adjusted < 0.f)
    //        Adjusted += Duration;
    //    ElapsedTime = StartTime + Adjusted;
    //}
    //else
    //{
    //    ElapsedTime = FMath::Clamp(ElapsedTime, StartTime, EndTime);

    //    if ((!bReverse && ElapsedTime >= EndTime) || (bReverse && ElapsedTime <= StartTime))
    //    {
    //        bPlaying = false;
    //        ElapsedTime = bReverse ? EndTime : StartTime;
    //    }
    //}



    CurrAnim->EvaluateAnimNotifies(CurrAnim->Notifies, ElapsedTime, PreviousTime, DeltaTime, SkeletalMeshComp, CurrAnim, bLooping);


    // 블렌딩 처리 (DeltaTime 누적 방식 없음 → BlendAlpha 자체 누적)
    float BlendAlphaValue = 1.f;
    if (bIsBlending && PrevAnim)
    {
        BlendAlpha += DeltaSeconds / BlendDuration;
        BlendAlpha = FMath::Clamp(BlendAlpha, 0.f, 1.f);

        if (BlendAlpha >= 1.f)
        {
            bIsBlending = false;
            PrevAnim = CurrAnim;
        }

        BlendAlphaValue = BlendAlpha;
    }

    const FReferenceSkeleton& RefSkeleton = this->GetCurrentSkeleton()->GetReferenceSkeleton();
    const int32 BoneNum = RefSkeleton.RawRefBoneInfo.Num();

    if (!PrevAnim || BoneNum <= 0 ||
        CurrAnim->GetSkeleton()->GetReferenceSkeleton().GetRawBoneNum() != BoneNum ||
        PrevAnim->GetSkeleton()->GetReferenceSkeleton().GetRawBoneNum() != BoneNum)
        return;

    FPoseContext PrevPose(this);
    FPoseContext CurrPose(this);
    PrevPose.Pose.InitBones(BoneNum);
    CurrPose.Pose.InitBones(BoneNum);

    for (int32 BoneIdx = 0; BoneIdx < BoneNum; ++BoneIdx)
    {
        PrevPose.Pose[BoneIdx] = RefSkeleton.RawRefBonePose[BoneIdx];
        CurrPose.Pose[BoneIdx] = RefSkeleton.RawRefBonePose[BoneIdx];
    }

    FAnimExtractContext Extract(ElapsedTime, false);
    CurrAnim->GetAnimationPose(CurrPose, Extract);
    PrevAnim->GetAnimationPose(PrevPose, Extract);

    FAnimationRuntime::BlendTwoPosesTogether(CurrPose.Pose, PrevPose.Pose, BlendAlphaValue, OutPose.Pose);
}


void ULuaScriptAnimInstance::SetAnimation(UAnimSequence* NewAnim, float BlendingTime, bool LoopAnim, bool ReverseAnim)
{
    if (CurrAnim == NewAnim)
    {
        return; // 이미 같은 애니메이션이 설정되어 있다면 아무 작업도 하지 않음.
    }

    if (!PrevAnim && !CurrAnim)
    {
        PrevAnim = NewAnim;
        CurrAnim = NewAnim;
    }
    else if (PrevAnim == nullptr)
    {
        PrevAnim = CurrAnim; // 이전 애니메이션이 없으면 현재 애니메이션을 이전으로 설정.
    }
    else if (CurrAnim)
    {
        PrevAnim = CurrAnim; // 현재 애니메이션이 있으면 현재를 이전으로 설정.
    }

    CurrAnim = NewAnim;
    BlendDuration = BlendingTime;
    bLooping = LoopAnim;
    bReverse = ReverseAnim;

    //ElapsedTime = 0.0f;
    BlendStartTime = ElapsedTime;
    BlendAlpha = 0.0f;
    bIsBlending = true;
    bPlaying = true;
}
