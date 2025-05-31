#include "AnimInstance.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "UObject/Casts.h"

void UAnimInstance::InitializeAnimation()
{
    USkeletalMeshComponent* OwnerComponent = GetSkelMeshComponent();
    if (OwnerComponent->GetSkeletalMeshAsset() != nullptr)
    {
        CurrentSkeleton = OwnerComponent->GetSkeletalMeshAsset()->GetSkeleton();
    }
    else
    {
        CurrentSkeleton = nullptr;
    }
}

void UAnimInstance::UpdateAnimation(float DeltaSeconds, FPoseContext& OutPose)
{
    NativeUpdateAnimation(DeltaSeconds, OutPose);
}

void UAnimInstance::NativeInitializeAnimation()
{
}

void UAnimInstance::NativeUpdateAnimation(float DeltaSeconds, FPoseContext& OutPose)
{
}

USkeletalMeshComponent* UAnimInstance::GetSkelMeshComponent() const
{
    return Cast<USkeletalMeshComponent>(GetOuter());
}
