#include "SocketComponent.h"

#include "SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"

USocketComponent::USocketComponent()
{
}

void USocketComponent::BeginPlay()
{
    USceneComponent::BeginPlay();
    // SkeletalMeshComponent = static_cast<USkeletalMeshComponent*>(GEngine->DuplicationMap[SkeletalMeshComponent]);
}

void USocketComponent::TickComponent(float DeltaTime)
{
    USceneComponent::TickComponent(DeltaTime);
    if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(GetAttachParent()))
    {
        const FTransform SocketTransform = SkeletalMeshComponent->GetSocketTransform(Socket);
        SetRelativeRotation(SocketTransform.GetRotation().Rotator());
        SetRelativeLocation(SocketTransform.GetTranslation());
    }
}

FReferenceSkeleton& USocketComponent::GetRefSkeletal() const
{
    return Cast<USkeletalMeshComponent>(GetAttachParent())->GetSkeletalMeshAsset()->GetRefSkeleton();
}

UObject* USocketComponent::Duplicate(UObject* InOuter)
{
    ThisClass* NewActor = Cast<ThisClass>(Super::Duplicate(InOuter));

    NewActor->Socket = Socket;
    //복제된 SkeletalMeshComponent를 찾기위한 열쇠임. 진짜는 BeginPlay에서 가져옴
    // NewActor->SkeletalMeshComponent = SkeletalMeshComponent;

    return NewActor;
}
