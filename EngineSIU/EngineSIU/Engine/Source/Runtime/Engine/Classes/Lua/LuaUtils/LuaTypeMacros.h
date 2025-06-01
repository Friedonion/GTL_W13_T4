#pragma once

#include "sol/sol.hpp"

// Class에서 LuaType 자동 등록 매크로.
#define DEFINE_LUA_TYPE_NO_PARENT(CLASS_NAME, ...)                                  \
static bool bRegisteredLuaProperties = false;                                       \
if (!bRegisteredLuaProperties)                                                      \
{                                                                                   \
    Lua.new_usertype<CLASS_NAME>(#CLASS_NAME,                                       \
        sol::constructors<CLASS_NAME()>(), ##__VA_ARGS__);                          \
    bRegisteredLuaProperties = true;                                                \
}

// 부모가 있는 경우 이 매크로로 LuaType 등록.
#define DEFINE_LUA_TYPE_WITH_PARENT(CLASS_NAME, BASE_CLASSES, ...)                 \
Super::RegisterLuaType(Lua);                                                       \
static bool bRegisteredLuaProperties = false;                                      \
if (!bRegisteredLuaProperties)                                                     \
{                                                                                  \
    Lua.new_usertype<CLASS_NAME>(#CLASS_NAME,                                      \
        sol::constructors<CLASS_NAME()>(),                                         \
        sol::base_classes, BASE_CLASSES, ##__VA_ARGS__);                           \
    bRegisteredLuaProperties = true;                                               \
}
