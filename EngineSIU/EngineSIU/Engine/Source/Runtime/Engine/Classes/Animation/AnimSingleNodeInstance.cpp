#include "AnimSingleNodeInstance.h"

#include "Components/SkeletalMeshComponent.h"
#include "AnimationAsset.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/AnimTypes.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimData/AnimDataModel.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "Misc/FrameTime.h"
#include "UObject/Casts.h"

UAnimSingleNodeInstance::UAnimSingleNodeInstance()
    : CurrentAsset(nullptr)
    , ElapsedTime(0.f)
    , PlayRate(1.f)
    , bLooping(true)
    , bPlaying(true)
    , bReverse(false)
    , PreviousTime(0.f)
    , LoopStartFrame(0)
    , LoopEndFrame(0)
    , CurrentKey(0)
{
}

void UAnimSingleNodeInstance::SetAnimationAsset(UAnimationAsset* NewAsset, bool bIsLooping, float InPlayRate)
{
    if (NewAsset != CurrentAsset)
    {
        CurrentAsset = NewAsset;
    }

    USkeletalMeshComponent* MeshComponent = GetSkelMeshComponent();
    if (MeshComponent)
    {
        if (MeshComponent->GetSkeletalMeshAsset() == nullptr)
        {
            CurrentAsset = nullptr;
        }
        else if (CurrentAsset != nullptr)
        {
            if (CurrentAsset->GetSkeleton() == nullptr)
            {
                CurrentAsset = nullptr;
            }
        }
    }
    
    bLooping = bIsLooping;
    PlayRate = InPlayRate;
    ElapsedTime = 0.f;

    LoopStartFrame = 0;

    if (UAnimSequence* AnimSequence = Cast<UAnimSequence>(CurrentAsset))
    {
        LoopEndFrame = AnimSequence->GetDataModel()->GetNumberOfFrames();
    }
}

void UAnimSingleNodeInstance::NativeInitializeAnimation()
{
}

void UAnimSingleNodeInstance::NativeUpdateAnimation(float DeltaSeconds, FPoseContext& OutPose)
{
    UAnimInstance::NativeUpdateAnimation(DeltaSeconds, OutPose);
    
    USkeletalMeshComponent* SkeletalMeshComp = GetSkelMeshComponent();
    
    if (!SkeletalMeshComp->GetAnimation() || !SkeletalMeshComp->GetSkeletalMeshAsset() || !SkeletalMeshComp->GetSkeletalMeshAsset()->GetSkeleton())
    {
        return;
    }

    UAnimSequence* AnimSequence = Cast<UAnimSequence>(CurrentAsset);
    const UAnimDataModel* DataModel = AnimSequence->GetDataModel();
    const int32 FrameRate = DataModel->GetFrameRate();
    const int32 NumberOfFrames = DataModel->GetNumberOfFrames();
    
    LoopStartFrame = FMath::Clamp(LoopStartFrame, 0, NumberOfFrames - 2);
    LoopEndFrame = FMath::Clamp(LoopEndFrame, LoopStartFrame + 1, NumberOfFrames - 1);
    const float StartTime = static_cast<float>(LoopStartFrame) / static_cast<float>(FrameRate);
    const float EndTime   = static_cast<float>(LoopEndFrame) / static_cast<float>(FrameRate);

    if (bPlaying)
    {
        float DeltaPlayTime = DeltaSeconds * PlayRate;
        if (bReverse)
        {
            DeltaPlayTime *= -1.0f;
        }

        PreviousTime = ElapsedTime;
        ElapsedTime += DeltaPlayTime;
        
        AnimSequence->EvaluateAnimNotifies(AnimSequence->Notifies, ElapsedTime, PreviousTime, DeltaPlayTime, SkeletalMeshComp, AnimSequence, bLooping);
        
        // 루프 처리
        if (IsLooping())
        {
            if (ElapsedTime > EndTime)
            {
                ElapsedTime = StartTime + FMath::Fmod(ElapsedTime - StartTime, EndTime - StartTime);
            }
            else if (ElapsedTime <= StartTime)
            {
                ElapsedTime = EndTime - FMath::Fmod(EndTime - ElapsedTime, EndTime - StartTime);
            }
        }
        else
        {
            if (!bReverse && ElapsedTime >= EndTime)
            {
                ElapsedTime = StartTime;
                SkeletalMeshComp->SetPlaying(false);
            }
            else if (bReverse && ElapsedTime <= StartTime)
            {
                ElapsedTime = EndTime;
                SkeletalMeshComp->SetPlaying(false);
            }
        }
    }

    // 본 트랜스폼 보간
    float TargetKeyFrame = ElapsedTime * static_cast<float>(FrameRate);
    const int32 CurrentFrame = static_cast<int32>(TargetKeyFrame) % (NumberOfFrames - 1);
    float Alpha = TargetKeyFrame - static_cast<float>(CurrentFrame);
    FFrameTime FrameTime(CurrentFrame, Alpha);
    
    CurrentKey = CurrentFrame;
    
    const FReferenceSkeleton& RefSkeleton = GetCurrentSkeleton()->GetReferenceSkeleton();

    // TODO: 인덱스 말고 맵을 통해 FName으로 포즈 계산
    for (int32 BoneIdx = 0; BoneIdx < RefSkeleton.RawRefBoneInfo.Num(); ++BoneIdx)
    {
        FName BoneName = RefSkeleton.RawRefBoneInfo[BoneIdx].Name;
        FTransform RefBoneTransform = RefSkeleton.RawRefBonePose[BoneIdx];
        OutPose.Pose[BoneIdx] = RefBoneTransform * DataModel->EvaluateBoneTrackTransform(BoneName, FrameTime, EAnimInterpolationType::Linear);
    }
}
