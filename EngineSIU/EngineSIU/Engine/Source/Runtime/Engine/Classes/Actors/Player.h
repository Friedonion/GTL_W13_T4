#pragma once

#include "GameFramework/Actor.h"

class USkeletalMeshComponent;
class UCameraComponent;

class APlayer : public AActor
{
    DECLARE_CLASS(APlayer, AActor)

public:
    APlayer() = default;

    virtual void BeginPlay() override;

    virtual UObject* Duplicate(UObject* InOuter) override;
    virtual void Tick(float DeltaTime) override;

    virtual void RegisterLuaType(sol::state& Lua); // Lua에 클래스 등록해주는 함수.
};

#pragma region W10
class ASequencerPlayer : public APlayer
{
    DECLARE_CLASS(ASequencerPlayer, APlayer)

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
