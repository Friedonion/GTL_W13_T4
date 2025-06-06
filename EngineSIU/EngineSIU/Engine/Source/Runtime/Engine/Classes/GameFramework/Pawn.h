#pragma once
#include "GameFramework/Actor.h"


class APawn : public AActor
{
    DECLARE_CLASS(APawn, AActor)

public:
    APawn() = default;
    
    virtual void PostSpawnInitialize();

    virtual UObject* Duplicate(UObject* InOuter) override;

    virtual void BeginPlay();

    virtual void Tick(float DeltaTime);

    virtual void Destroyed();

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason);

    virtual void RegisterLuaType(sol::state& Lua); // Lua에 클래스 등록해주는 함수.

};
