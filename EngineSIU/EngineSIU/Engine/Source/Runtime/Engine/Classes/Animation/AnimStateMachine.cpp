#include "AnimStateMachine.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/Contents/AnimInstance/LuaScriptAnimInstance.h"
#include "Lua/LuaScriptManager.h"
#include "Animation/AnimSequence.h"

UAnimStateMachine::UAnimStateMachine()
{
    
}

void UAnimStateMachine::Initialize(USkeletalMeshComponent* InOwner, ULuaScriptAnimInstance* InAnimInstance)
{
    OwningComponent = InOwner;
    OwningAnimInstance = InAnimInstance;

    LuaScriptName = OwningComponent->StateMachineFileName;
    InitLuaStateMachine();
}

void UAnimStateMachine::ProcessState()
{
    if (!LuaTable.valid())
        return;

    sol::function UpdateFunc = LuaTable["Update"];
    if (!UpdateFunc.valid())
    {
        UE_LOG(ELogLevel::Warning, TEXT("Lua Update function not valid!"));
        return;
    }

    sol::object result = UpdateFunc(LuaTable, 0.0f);

    sol::table StateInfo = result.as<sol::table>();
    FString StateName = StateInfo["anim"].get_or(std::string("")).c_str();
    float Blend = StateInfo["blend"].get_or(0.f);

    if (OwningAnimInstance)
    {
        UAnimSequence* NewAnim = Cast<UAnimSequence>(UAssetManager::Get().GetAnimation(StateName));
        OwningAnimInstance->SetAnimation(NewAnim, Blend, false, false);
    }
}

void UAnimStateMachine::InitLuaStateMachine()
{
    if (LuaScriptName.IsEmpty())
    {
        return;
    }
    LuaTable = FLuaScriptManager::Get().CreateLuaTable(LuaScriptName);

    FLuaScriptManager::Get().RegisterActiveAnimLua(this);
    if (!LuaTable.valid())
        return;
}



