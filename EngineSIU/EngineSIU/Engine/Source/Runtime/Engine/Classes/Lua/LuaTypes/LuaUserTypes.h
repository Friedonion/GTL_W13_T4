#pragma once
#include "sol/sol.hpp"

struct FMatrix;
struct FQuat;
struct FRotator;
struct FVector4;
struct FVector2D;
struct FVector;
struct FLinearColor;
struct FColor;

namespace LuaTypes
{
    template <typename T>
    struct FBindLua
    {
        static void Bind([[maybe_unused]] sol::table& Table)
        {
            static_assert(sizeof(T) == 0, "Binding not implemented for this type!");
        }
    };

    // Math Types
    template <> struct FBindLua<FColor> { static void Bind(sol::table& Table); };
    template <> struct FBindLua<FLinearColor> { static void Bind(sol::table& Table); };
    template <> struct FBindLua<FVector> { static void Bind(sol::table& Table); };
    template <> struct FBindLua<FVector2D> { static void Bind(sol::table& Table); };
    template <> struct FBindLua<FVector4> { static void Bind(sol::table& Table); };
    template <> struct FBindLua<FRotator> { static void Bind(sol::table& Table); };
    template <> struct FBindLua<FQuat> { static void Bind(sol::table& Table); };
    template <> struct FBindLua<FMatrix> { static void Bind(sol::table& Table); };

}
