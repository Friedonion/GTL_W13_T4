#pragma once
#include "GameFramework/Actor.h"

class UCameraComponent;
class USkeletalMeshComponent;

class ASequencerPlayer : public AActor
{
    DECLARE_CLASS(ASequencerPlayer, AActor)

public:
    ASequencerPlayer();
    virtual ~ASequencerPlayer() override = default;

    virtual void PostSpawnInitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual UObject* Duplicate(UObject* InOuter) override;

    FName Socket = "jx_c_camera";
    USkeletalMeshComponent* SkeletalMeshComponent = nullptr;

private:
    UCameraComponent* CameraComponent = nullptr;
};
#pragma endregion
