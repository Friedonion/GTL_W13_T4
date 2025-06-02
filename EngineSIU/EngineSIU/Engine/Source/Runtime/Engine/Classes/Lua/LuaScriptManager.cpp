#include "LuaScriptManager.h"
#include <filesystem>
#include <UserInterface/Console.h>

#include "Lua/LuaTypes/LuaUserTypes.h"
#include "Lua/LuaScriptComponent.h"
#include "Animation/AnimStateMachine.h"
#include "GameFramework/Actor.h"

#include "LuaScripts/LuaUIBind.h"

TMap<FString, FLuaTableScriptInfo> FLuaScriptManager::ScriptCacheMap;
TSet<ULuaScriptComponent*> FLuaScriptManager::ActiveLuaComponents;
TSet<UAnimStateMachine*> FLuaScriptManager::ActiveAnimLua;

FLuaScriptManager::FLuaScriptManager()
{
    LuaState.open_libraries(
        sol::lib::base,       // Lua를 사용하기 위한 기본 라이브러리 (print, type, pcall 등)
        // sol::lib::package,    // 모듈 로딩(require) 및 패키지 관리 기능
        sol::lib::coroutine,  // 코루틴 생성 및 관리 기능 (yield, resume 등)
        sol::lib::string,     // 문자열 검색, 치환, 포맷팅 등 문자열 처리 기능
        sol::lib::math,       // 수학 함수 (sin, random, pi 등) 및 상수 제공
        sol::lib::table,      // 테이블(배열/딕셔너리) 생성 및 조작 기능 (insert, sort 등)
        // sol::lib::io,         // 파일 읽기/쓰기 등 입출력 관련 기능
        // sol::lib::os,         // 운영체제 관련 기능 (시간, 날짜, 파일 시스템 접근 등)
        sol::lib::debug,      // 디버깅 및 introspection 기능 (traceback, getinfo 등)
        sol::lib::bit32,      // 32비트 정수 대상 비트 연산 기능 (Lua 5.2 이상)
        // sol::lib::jit,        // LuaJIT의 JIT 컴파일러 제어 기능 (LuaJIT 전용)
        // sol::lib::ffi,        // 외부 C 함수 및 데이터 구조 접근 기능 (LuaJIT 전용)
        sol::lib::utf8        // UTF-8 인코딩 문자열 처리 기능 (Lua 5.3 이상)
    );

    SetLuaDefaultTypes();
}

FLuaScriptManager::~FLuaScriptManager()
{
    ScriptCacheMap.Empty();
    ActiveLuaComponents.Empty();
    ActiveAnimLua.Empty();
}

void FLuaScriptManager::SetLuaDefaultTypes()
{
    sol::table TypeTable = LuaState.create_table("EngineTypes");

    // Default Math Types.
    LuaTypes::FBindLua<FColor>::Bind(TypeTable);
    LuaTypes::FBindLua<FLinearColor>::Bind(TypeTable);
    LuaTypes::FBindLua<FVector>::Bind(TypeTable);
    LuaTypes::FBindLua<FVector2D>::Bind(TypeTable);
    LuaTypes::FBindLua<FVector4>::Bind(TypeTable);
    LuaTypes::FBindLua<FRotator>::Bind(TypeTable);
    LuaTypes::FBindLua<FQuat>::Bind(TypeTable);
    LuaTypes::FBindLua<FMatrix>::Bind(TypeTable);
    LuaTypes::FBindLua<FString>::Bind(TypeTable);

    LuaUIBind::Bind(TypeTable);
}

FLuaScriptManager& FLuaScriptManager::Get()
{
    static FLuaScriptManager Instance;
    return Instance;
}

sol::state& FLuaScriptManager::GetLua()
{
    return LuaState;
}

sol::table FLuaScriptManager::CreateLuaTable(const FString& ScriptName)
{
    if (!std::filesystem::exists(*ScriptName))
    {
        UE_LOG(ELogLevel::Error, TEXT("InValid Lua File name."));
        //assert(false && "Lua File not found");
        return sol::table();
    }

    if (!ScriptCacheMap.Contains(ScriptName))
    {
        sol::protected_function_result Result = LuaState.script_file(*ScriptName);
        if (!Result.valid())
        {
            sol::error err = Result;
            UE_LOG(ELogLevel::Error, TEXT("Lua Error: %s"), *FString(err.what()));
            return sol::table();
        }
        
        sol::object ReturnValue = Result.get<sol::object>();
        if (!ReturnValue.is<sol::table>())
        {
            UE_LOG(ELogLevel::Error, TEXT("Lua Error: %s"), *FString("Script file did not return a table."));
            return sol::table();
        }
        
        FLuaTableScriptInfo NewInfo;
        NewInfo.ScriptTable = ReturnValue.as<sol::table>();
        NewInfo.LastWriteTime = std::filesystem::last_write_time(ScriptName.ToWideString());
        ScriptCacheMap.Add(ScriptName, NewInfo);
    }

    //return ScriptCacheMap[ScriptName];

    sol::table& ScriptClass = ScriptCacheMap[ScriptName].ScriptTable;

    sol::table NewEnv = LuaState.create_table();
    for (auto& pair : ScriptClass)
    {
        NewEnv.set(pair.first, pair.second);
    }

    return NewEnv;
}

void FLuaScriptManager::RegisterActiveLuaComponent(ULuaScriptComponent* LuaComponent)
{
    ActiveLuaComponents.Add(LuaComponent);
}

void FLuaScriptManager::UnRigisterActiveLuaComponent(ULuaScriptComponent* LuaComponent)
{
    if (ActiveLuaComponents.Contains(LuaComponent))
        ActiveLuaComponents.Remove(LuaComponent);
}

void FLuaScriptManager::RegisterActiveAnimLua(UAnimStateMachine* AnimInstance)
{
    ActiveAnimLua.Add(AnimInstance);
}

void FLuaScriptManager::UnRegisterActiveAnimLua(UAnimStateMachine* AnimInstance)
{
    if (ActiveAnimLua.Contains(AnimInstance))
        ActiveAnimLua.Remove(AnimInstance);
}

void FLuaScriptManager::HotReloadLuaScript()
{
    TSet<FString> ChangedScriptName;
    TMap<FString, FLuaTableScriptInfo> CopiedScriptCacheMap = ScriptCacheMap;
    for (const TMap<FString, FLuaTableScriptInfo>::PairType& ScriptCached : CopiedScriptCacheMap)
    {
        FString ScriptName = ScriptCached.Key;
        FLuaTableScriptInfo LuaScriptInfo = ScriptCached.Value;
        auto depTime = std::filesystem::last_write_time(GetData(ScriptName));
        if (LuaScriptInfo.LastWriteTime != depTime)
        {
            if (ScriptCacheMap.Contains(ScriptName))
            {
                ScriptCacheMap.Remove(ScriptName);
            }

            ChangedScriptName.Add(ScriptName);
        }
    }

    for (const FString& ChangedScript : ChangedScriptName)
    {
        for (const ULuaScriptComponent* LuaComponent : ActiveLuaComponents)
        {
            const FString FullPath = "LuaScripts/Actors/" + LuaComponent->GetScriptName() + ".lua";
            if (FullPath == ChangedScript)
            {
                LuaComponent->GetOwner()->BindSelfLuaProperties();
                UE_LOG(ELogLevel::Display, TEXT("Lua Script Reloaded: %s"), *ChangedScript);
            } 
        }

        for (UAnimStateMachine* AnimStateMachine : ActiveAnimLua)
        {
            if (AnimStateMachine->GetLuaScriptName() == ChangedScript)
            {
                AnimStateMachine->InitLuaStateMachine();
                UE_LOG(ELogLevel::Display, TEXT("Lua Script Reloaded: %s"), *ChangedScript);
            }
        }
    }
}
