#pragma once

#include "Container/Set.h"
#include "Container/Map.h"
#include "Container/String.h"
#include "sol/sol.hpp"

#include <filesystem>

class ULuaScriptComponent;
class UAnimStateMachine;

struct FLuaTableScriptInfo
{
    sol::table ScriptTable;
    std::filesystem::file_time_type LastWriteTime;
};

class FLuaScriptManager
{

private:
    sol::state LuaState;
    static TMap<FString, FLuaTableScriptInfo> ScriptCacheMap;
    static TSet<ULuaScriptComponent*> ActiveLuaComponents;
    static TSet<UAnimStateMachine*> ActiveAnimLua;

public:
    FLuaScriptManager();
    ~FLuaScriptManager();

private:
    void SetLuaDefaultTypes();

public:

    static FLuaScriptManager& Get();

    sol::state& GetLua();
    sol::table CreateLuaTable(const FString& ScriptName);

    void RegisterActiveLuaComponent(ULuaScriptComponent* LuaComponent);
    void UnRigisterActiveLuaComponent(ULuaScriptComponent* LuaComponent);

    void RegisterActiveAnimLua(UAnimStateMachine* AnimInstance);
    void UnRegisterActiveAnimLua(UAnimStateMachine* AnimInstance);

    void HotReloadLuaScript();

};

