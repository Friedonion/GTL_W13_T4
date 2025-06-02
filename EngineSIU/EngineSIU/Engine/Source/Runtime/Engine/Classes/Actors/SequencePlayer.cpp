#include "SequencePlayer.h"
#include "Components/SkeletalMeshComponent.h"
#include "Classes/Camera/CameraComponent.h"

ASequencerPlayer::ASequencerPlayer()
{
}

void ASequencerPlayer::PostSpawnInitialize()
{
    Super::PostSpawnInitialize();

    RootComponent = AddComponent<USceneComponent>();

    CameraComponent = AddComponent<UCameraComponent>();
    CameraComponent->SetupAttachment(RootComponent);
}

void ASequencerPlayer::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (SkeletalMeshComponent)
    {
        const FTransform SocketTransform = SkeletalMeshComponent->GetSocketTransform(Socket);
        SetActorRotation(SocketTransform.GetRotation().Rotator());
        SetActorLocation(SocketTransform.GetTranslation());
    }
}

UObject* ASequencerPlayer::Duplicate(UObject* InOuter)
{
    ThisClass* NewActor = Cast<ThisClass>(Super::Duplicate(InOuter));

    NewActor->Socket = Socket;
    NewActor->SkeletalMeshComponent = nullptr;

    return NewActor;
}
