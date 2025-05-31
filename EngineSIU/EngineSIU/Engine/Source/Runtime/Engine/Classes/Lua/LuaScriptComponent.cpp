#include "LuaScriptComponent.h"

#include "Lua/LuaScriptManager.h"
#include "GameFramework/Actor.h"

#include "World/World.h"

ULuaScriptComponent::ULuaScriptComponent()
{
}

UObject* ULuaScriptComponent::Duplicate(UObject* InOuter)
{
    ULuaScriptComponent* NewComponent = Cast<ULuaScriptComponent>(Super::Duplicate(InOuter));

    if (!NewComponent)
    {
        return nullptr;
    }

    NewComponent->ScriptName = ScriptName;
    NewComponent->SelfTable = SelfTable;
 
    return NewComponent;
}

void ULuaScriptComponent::GetProperties(TMap<FString, FString>& OutProperties) const
{
    Super::GetProperties(OutProperties);

    OutProperties.Add(("ScriptName"), *ScriptName);
}

void ULuaScriptComponent::SetProperties(const TMap<FString, FString>& Properties)
{
    Super::SetProperties(Properties);
    const FString* TempStr = nullptr;
    
    TempStr = Properties.Find(TEXT("ScriptName"));
    if (TempStr)
    {
        ScriptName = *TempStr;
    }
}

void ULuaScriptComponent::InitializeComponent()
{
    if (HasBeenInitialized())
    {
        return;
    }
    Super::InitializeComponent();
    FLuaScriptManager::Get().RegisterActiveLuaComponent(this);
}

void ULuaScriptComponent::BeginPlay()
{
    Super::BeginPlay();

    if (SelfTable.valid() && SelfTable["BeginPlay"].valid())
    {
        ActivateFunction("BeginPlay", SelfTable);
    }
}

void ULuaScriptComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);
    if (SelfTable.valid() && SelfTable["Tick"].valid())
    {
        ActivateFunction("Tick", DeltaTime);
    }
}

void ULuaScriptComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (SelfTable.valid() && SelfTable["EndPlay"].valid())
    {
        ActivateFunction("EndPlay", EndPlayReason);
    }
}

void ULuaScriptComponent::DestroyComponent(bool bPromoteChildren)
{
    FLuaScriptManager::Get().UnRigisterActiveLuaComponent(this);
    Super::DestroyComponent(bPromoteChildren);
    SelfTable.reset(); // Lua 테이블 초기화
}

bool ULuaScriptComponent::LoadScript()
{
    // ScriptName 이 없으면 등록 안함.
    if (ScriptName.IsEmpty())
    {
        return false;
    }

    const FString ScriptFullName = "LuaScripts/Actors/" + ScriptName + ".lua";
    
    SelfTable = FLuaScriptManager::Get().CreateLuaTable(ScriptFullName);

        if (!SelfTable.valid())
    {
        return false;
    }
    return true;
}
