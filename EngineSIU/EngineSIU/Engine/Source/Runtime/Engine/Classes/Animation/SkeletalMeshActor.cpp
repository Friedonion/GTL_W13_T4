#include "SkeletalMeshActor.h"

#include "Components/SkeletalMeshComponent.h"

void ASkeletalMeshActor::PostSpawnInitialize()
{
    Super::PostSpawnInitialize();

    SkeletalMeshComponent = AddComponent<USkeletalMeshComponent>("SkeletalMeshComponent");
}

void ASkeletalMeshActor::BeginPlay()
{
    Super::BeginPlay();
}

UObject* ASkeletalMeshActor::Duplicate(UObject* InOuter)
{
    ASkeletalMeshActor* NewActor = Cast<ASkeletalMeshActor>(Super::Duplicate(InOuter));

    NewActor->SkeletalMeshComponent = NewActor->GetComponentByClass<USkeletalMeshComponent>();

    return NewActor;
}
