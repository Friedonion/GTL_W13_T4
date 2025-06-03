#pragma once
#include "Pawn.h"

class UCapsuleComponent;
class USkeletalMeshComponent;

class ACharacter : public APawn
{
    DECLARE_CLASS(ACharacter, APawn)

public:
    ACharacter();
    
    virtual void PostSpawnInitialize();

    virtual UObject* Duplicate(UObject* InOuter) override;

    virtual void BeginPlay();

    virtual void Tick(float DeltaTime);

    virtual void Destroyed();

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason);

    virtual void RegisterLuaType(sol::state& Lua); // Lua에 클래스 등록해주는 함수.
    
    UPROPERTY(
        EditAnywhere,
        float,
        BaseEyeHeight,
        = 0.0f
    )
protected:
    UPROPERTY(
        VisibleAnywhere,
        USkeletalMeshComponent*,
        Mesh,
        = nullptr
    )

    UPROPERTY(
        VisibleAnywhere,
        UCapsuleComponent*,
        CapsuleComponent,
        = nullptr
    )

};
