#include "LuaUserTypes.h"

#include "Engine/World/World.h"
#include "Lua/LuaUtils/LuaBindUtils.h"
#include "Math/Color.h"
#include "Math/Matrix.h"
#include "Math/Quat.h"

void LuaTypes::FBindLua<FColor>::Bind(sol::table& Table)
{
    Table.Lua_NewUserType(
        FColor,

        // 생성자
        sol::constructors<FColor(), FColor(uint8, uint8, uint8, uint8)>(),

        // 멤버 변수
        LUA_BIND_MEMBER(&FColor::R),
        LUA_BIND_MEMBER(&FColor::G),
        LUA_BIND_MEMBER(&FColor::B),
        LUA_BIND_MEMBER(&FColor::A),

        // 연산자 오버로딩
        sol::meta_function::equal_to, &FColor::operator==,

        // 정적 상수 (읽기 전용 속성으로 바인딩)
        LUA_BIND_STATIC(FColor::White),
        LUA_BIND_STATIC(FColor::Black),
        LUA_BIND_STATIC(FColor::Transparent),
        LUA_BIND_STATIC(FColor::Red),
        LUA_BIND_STATIC(FColor::Green),
        LUA_BIND_STATIC(FColor::Blue),
        LUA_BIND_STATIC(FColor::Yellow)
    );
}

void LuaTypes::FBindLua<FLinearColor>::Bind(sol::table& Table)
{
    Table.Lua_NewUserType(
        FLinearColor,

        // Constructors
        sol::constructors<FLinearColor(), FLinearColor(float, float, float, float)>(),

        // Member variables
        LUA_BIND_MEMBER(&FLinearColor::R),
        LUA_BIND_MEMBER(&FLinearColor::G),
        LUA_BIND_MEMBER(&FLinearColor::B),
        LUA_BIND_MEMBER(&FLinearColor::A),

        // Operators
        sol::meta_function::equal_to, &FLinearColor::operator==,
        sol::meta_function::multiplication, [](const FLinearColor& A, const FLinearColor& B) { return A * B; },
        sol::meta_function::addition, &FLinearColor::operator+,

        // Static constants
        LUA_BIND_STATIC(FLinearColor::White),
        LUA_BIND_STATIC(FLinearColor::Black),
        LUA_BIND_STATIC(FLinearColor::Transparent),
        LUA_BIND_STATIC(FLinearColor::Red),
        LUA_BIND_STATIC(FLinearColor::Green),
        LUA_BIND_STATIC(FLinearColor::Blue),
        LUA_BIND_STATIC(FLinearColor::Yellow)
    );
}

void LuaTypes::FBindLua<FVector>::Bind(sol::table& Table)
{
    Table.Lua_NewUserType(
        FVector,
        sol::call_constructor,
        // Constructors
        sol::constructors<
        FVector(),
        FVector(float, float, float),
        FVector(float)
        >(),

        // Member variables
        LUA_BIND_MEMBER(&FVector::X),
        LUA_BIND_MEMBER(&FVector::Y),
        LUA_BIND_MEMBER(&FVector::Z),

        // Operators
        sol::meta_function::equal_to, &FVector::operator==,
        sol::meta_function::addition, sol::overload(
        sol::resolve<FVector(const FVector&) const>(&FVector::operator+),  // FVector + FVector
        sol::resolve<FVector(float) const>(&FVector::operator+)            // FVector + float
    ),
        sol::meta_function::subtraction, [](const FVector& A, const FVector& B) { return A - B; },
        sol::meta_function::multiplication, sol::overload(
            sol::resolve<FVector(const FVector&) const>(&FVector::operator*),
            sol::resolve<FVector(float) const>(&FVector::operator*)
        ),
        sol::meta_function::division, [](const FVector& A, const FVector& B) { return A / B; },

        // Utility functions
        LUA_BIND_MEMBER(&FVector::Length),
        LUA_BIND_MEMBER(&FVector::SquaredLength),
        LUA_BIND_MEMBER(&FVector::Normalize),
        LUA_BIND_MEMBER(&FVector::IsNormalized),
        LUA_BIND_MEMBER(&FVector::DotProduct),
        LUA_BIND_MEMBER(&FVector::CrossProduct),

        // Static properties
        LUA_BIND_STATIC(FVector::ZeroVector),
        LUA_BIND_STATIC(FVector::OneVector),
        LUA_BIND_STATIC(FVector::UpVector),
        LUA_BIND_STATIC(FVector::DownVector),
        LUA_BIND_STATIC(FVector::ForwardVector),
        LUA_BIND_STATIC(FVector::BackwardVector),
        LUA_BIND_STATIC(FVector::RightVector),
        LUA_BIND_STATIC(FVector::LeftVector),
        LUA_BIND_STATIC(FVector::XAxisVector),
        LUA_BIND_STATIC(FVector::YAxisVector),
        LUA_BIND_STATIC(FVector::ZAxisVector)
    );
}

void LuaTypes::FBindLua<FVector2D>::Bind(sol::table& Table)
{
    Table.Lua_NewUserType(
        FVector2D,

        // Constructors
        sol::constructors<FVector2D(), FVector2D(float, float)>(),

        // Member variables
        LUA_BIND_MEMBER(&FVector2D::X),
        LUA_BIND_MEMBER(&FVector2D::Y),

        // Operators
        sol::meta_function::equal_to, &FVector2D::operator==,
        sol::meta_function::addition, &FVector2D::operator+,
        sol::meta_function::subtraction, &FVector2D::operator-,
        sol::meta_function::multiplication, &FVector2D::operator*,
        sol::meta_function::division, &FVector2D::operator/,

        // Utility functions
        // LUA_BIND_MEMBER(&FVector2D::Length),
        // LUA_BIND_MEMBER(&FVector2D::LengthSquared),
        // LUA_BIND_MEMBER(&FVector2D::Normalize),
        // LUA_BIND_MEMBER(&FVector2D::IsNormalized),
        // LUA_BIND_MEMBER(&FVector2D::DotProduct),

        // Static properties
        LUA_BIND_STATIC(FVector2D::ZeroVector)
        // LUA_BIND_STATIC(FVector2D::UnitVector)
    );
}

void LuaTypes::FBindLua<FVector4>::Bind(sol::table& Table)
{
    Table.Lua_NewUserType(
        FVector4,

        // Constructors
        sol::constructors<FVector4(), FVector4(float, float, float, float)>(),

        // Member variables
        LUA_BIND_MEMBER(&FVector4::X),
        LUA_BIND_MEMBER(&FVector4::Y),
        LUA_BIND_MEMBER(&FVector4::Z),
        LUA_BIND_MEMBER(&FVector4::W)
    );
}

void LuaTypes::FBindLua<FRotator>::Bind(sol::table& Table)
{
    Table.Lua_NewUserType(
        FRotator,

        // Constructors
        sol::constructors<FRotator(), FRotator(float, float, float)>(),

        // Member variables
        LUA_BIND_MEMBER(&FRotator::Pitch),
        LUA_BIND_MEMBER(&FRotator::Yaw),
        LUA_BIND_MEMBER(&FRotator::Roll),

        // Operators
        sol::meta_function::equal_to, &FRotator::operator==,
        sol::meta_function::addition, &FRotator::operator+,
        sol::meta_function::subtraction, [](const FRotator& A, const FRotator& B) { return A - B; },
        sol::meta_function::multiplication, &FRotator::operator*,

        // Utility functions
        LUA_BIND_MEMBER(&FRotator::Normalize),
        LUA_BIND_MEMBER(&FRotator::GetNormalized)
        // LUA_BIND_MEMBER(&FRotator::GetInverse),

        // Static properties
        // LUA_BIND_STATIC(FRotator::ZeroRotator)
    );
}

void LuaTypes::FBindLua<FQuat>::Bind(sol::table& Table)
{
    Table.Lua_NewUserType(
        FQuat,

        // Constructors
        sol::constructors<FQuat(), FQuat(float, float, float, float)>(),

        // Member variables
        LUA_BIND_MEMBER(&FQuat::X),
        LUA_BIND_MEMBER(&FQuat::Y),
        LUA_BIND_MEMBER(&FQuat::Z),
        LUA_BIND_MEMBER(&FQuat::W),

        // Utility functions
        LUA_BIND_MEMBER(&FQuat::Normalize),
        LUA_BIND_MEMBER(&FQuat::GetNormalized),
        LUA_BIND_MEMBER(&FQuat::IsNormalized),
        LUA_BIND_MEMBER(&FQuat::RotateVector)
        // LUA_BIND_MEMBER(&FQuat::GetAxisX),
        // LUA_BIND_MEMBER(&FQuat::GetAxisY),
        // LUA_BIND_MEMBER(&FQuat::GetAxisZ),

        // Static functions
        // LUA_BIND_STATIC(FQuat::Identity)
    );
}

void LuaTypes::FBindLua<FMatrix>::Bind(sol::table& Table)
{
    Table.Lua_NewUserType(
        FMatrix,

        // Constructors
        sol::constructors<FMatrix()>(),

        // Matrix operations
        // LUA_BIND_MEMBER(&FMatrix::Determinant),
        // LUA_BIND_MEMBER(&FMatrix::Inverse),
        // LUA_BIND_MEMBER(&FMatrix::Transpose),
        // LUA_BIND_MEMBER(&FMatrix::TransformVector),
        // LUA_BIND_MEMBER(&FMatrix::TransformPosition),

        // Static functions
        LUA_BIND_STATIC(FMatrix::Identity)
    );
}
