#pragma once
#include<UObject/Object.h>
#include "UObject/ObjectMacros.h"
#include "sol/sol.hpp"
#include "Lua/LuaScriptManager.h"

class USkeletalMeshComponent;
class ULuaScriptAnimInstance;
class APawn;

class UAnimStateMachine : public UObject
{
    DECLARE_CLASS(UAnimStateMachine, UObject)

public:
    UAnimStateMachine();
    virtual ~UAnimStateMachine() override = default;

    virtual void Initialize(USkeletalMeshComponent* InOwner, ULuaScriptAnimInstance* InAnimInstance);

    void ProcessState();
    
    template<typename T>
    typename std::enable_if<std::is_base_of<AActor, T>::value>::type
    InitLuaStateMachine(T* Owner);
    
    FString GetLuaScriptName() const { return LuaScriptName; }
    void SetLuaScriptName(const FString& InName) { LuaScriptName = InName; }

    USkeletalMeshComponent* OwningComponent;
    ULuaScriptAnimInstance* OwningAnimInstance;
    
private:
    UPROPERTY(EditAnywhere, FString, LuaScriptName, = TEXT(""));
    sol::table LuaTable = {};

};

template<typename T>
inline typename std::enable_if<std::is_base_of<AActor, T>::value>::type UAnimStateMachine::InitLuaStateMachine(T* Owner)
{
    if (LuaScriptName.IsEmpty())
    {
        return;
    }
    LuaTable = FLuaScriptManager::Get().CreateLuaTable(LuaScriptName);

    FLuaScriptManager::Get().RegisterActiveAnimLua(this);
    if (!LuaTable.valid())
        return;

    LuaTable["Owner"] = Owner;
}



