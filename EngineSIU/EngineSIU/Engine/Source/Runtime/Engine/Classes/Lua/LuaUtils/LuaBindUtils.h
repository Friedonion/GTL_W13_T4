#pragma once

#include <string_view>

namespace LuaBindUtils
{
    /**
     * 멤버 변수 포인터 형식의 문자열에서 멤버 변수 이름만 가져옵니다.
     * @note "&Test::Var" -> "Var"
     */
    consteval std::string_view GetMemberName(std::string_view RawName, bool bIsMemberMethod = false)
    {
        if (bIsMemberMethod)
        {
            // 멤버메서드가 아닌 경우
            if (!RawName.starts_with("&")) throw;
        }

        // 1. 마지막 '::' 찾기
        const size_t LastColonPos = RawName.find_last_of(':');
        if (LastColonPos == std::string_view::npos) throw;

        // 2. '::' 뒤부터 끝까지 잘라내기
        const std::string_view MemberName = RawName.substr(LastColonPos + 1);

        // 3. (선택적) 이름 유효성 검사 (예: 공백 없음 등)
        if (MemberName.empty()) throw;

        return MemberName;
    }
}

/**
 * Lua에 새로운 UserType을 추가합니다.
 * @param TypeName lua에서 사용될 커스텀 타입
 * @note Ex) sol::state Lua; Lua.Lua_New_UserType(FTestStruct, ...);
**/

#define Lua_NewUserType(TypeName, ...) new_usertype<TypeName>(#TypeName, __VA_ARGS__)

#define LUA_BIND_MEMBER(MemberName) LuaBindUtils::GetMemberName(#MemberName, true), MemberName

#define LUA_BIND_STATIC(MemberName) LuaBindUtils::GetMemberName(#MemberName, false), sol::var(MemberName)
