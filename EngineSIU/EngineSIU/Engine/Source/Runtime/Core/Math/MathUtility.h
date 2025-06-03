#pragma once
#include <cmath>
#include <concepts>
#include <numbers>
#include <random>
#include <type_traits>

#include "MathFwd.h"
#include "MathSSE.h"
#include "Core/HAL/PlatformType.h"


#define PI                   (3.1415926535897932f)
#define SMALL_NUMBER         (1.e-8f)
#define KINDA_SMALL_NUMBER   (1.e-4f)
#define INV_PI				 (0.31830988618f)
#define HALF_PI				 (1.57079632679f)
#define TWO_PI				 (6.28318530717f)
#define PI_SQUARED			 (9.86960440108f)

#define PI_DOUBLE            (3.141592653589793238462643383279502884197169399)


/**
 * 커스텀 타입에 대해서 Lerp를 사용할 수 있도록 합니다.
 * @tparam T 커스텀 타입
 *
 * template<>
 * TCustomLerp<MyClass>
 * {
 *     // SFINAE에 걸리지 않기 위해 반드시 true로 선언해야 합니다.
 *     constexpr static bool Value = true;
 *
 *     // Lerp 이름 (FMath에 있는 Lerp의 이름과 같아야함)
 *     static inline MyClass Lerp(const MyClass& A, const MyClass& B, float Alpha)
 *     {
 *         return MyClass::Lerp(A, B, Alpha); // Or do the computation here directly
 *     }
 * }
 */
template <typename T>
struct TCustomLerp
{
    constexpr static bool Value = false;
};

template <typename T>
concept TCustomLerpable = TCustomLerp<T>::Value;


struct FMath
{
    /** A와 B중에 더 작은 값을 반환합니다. */
    template <typename T>
    [[nodiscard]] static FORCEINLINE constexpr T Min(const T A, const T B)
    {
        return A < B ? A : B;
    }

    /** A와 B중에 더 큰 값을 반환합니다. */
    template <typename T>
    [[nodiscard]] static FORCEINLINE constexpr T Max(const T A, const T B)
    {
        return B < A ? A : B;
    }

    /** A, B, C 중에 가장 큰 값을 반환합니다. */
    template <typename T>
    [[nodiscard]] static FORCEINLINE constexpr T Max3(const T A, const T B, const T C)
    {
        return Max(A, Max(B, C));
    }

    /** X를 Min과 Max의 사이의 값으로 클램핑 합니다. */
    template <typename T>
    [[nodiscard]] static FORCEINLINE constexpr T Clamp(const T X, const T MinValue, const T MaxValue)
    {
        return Max(Min(X, MaxValue), MinValue);
    }

    /** A의 절댓값을 구합니다. */
    template <typename T>
    [[nodiscard]] static FORCEINLINE constexpr T Abs(const T A)
    {
        return A < T(0) ? -A : A;
    }

    /** Returns 1, 0, or -1 depending on relation of T to 0 */
    template <typename T>
    static constexpr FORCEINLINE T Sign(const T A)
    {
        return (A > (T)0) ? (T)1 : ((A < (T)0) ? (T)-1 : (T)0);
    }

    /** A의 제곱을 구합니다. */
    template <typename T>
    [[nodiscard]] static FORCEINLINE constexpr T Square(const T A) { return A * A; }

    /** A의 세제곱을 구합니다. */
    template <typename T>
    [[nodiscard]] static constexpr FORCEINLINE T Cube(const T A) { return A * A * A; }

    [[nodiscard]] static FORCEINLINE float Pow(float A, float B) { return powf(A, B); }
    [[nodiscard]] static FORCEINLINE double Pow(double A, double B) { return pow(A, B); }

    // A의 제곱근을 구합니다.
    [[nodiscard]] static FORCEINLINE float Sqrt(float A) { return sqrtf(A); }
    [[nodiscard]] static FORCEINLINE double Sqrt(double A) { return sqrt(A); }

    /** A의 역제곱근을 구합니다. */
    [[nodiscard]] static FORCEINLINE float InvSqrt(float A) { return 1.0f / sqrtf(A); }
    [[nodiscard]] static FORCEINLINE double InvSqrt(double A) { return 1.0 / sqrt(A); }

    /**
     * 비교값(Comparand)에 따라 값을 선택적으로 반환합니다. 
     * 이 함수의 주요 목적은 부동소수점 비교 시 발생하는 분기 예측을 회피하고
     * 컴파일러 내장 함수를 통해 최적화하는 것입니다.
     *
     * @param	Comparand		기준이 되는 비교값
     * @param	ValueGEZero		Comparand >= 0일 때 반환값
     * @param	ValueLTZero		Comparand < 0일 때 반환값
     * @return	Comparand >= 0이면 ValueGEZero, 그 외에는 ValueLTZero
     *
     * @warning NaN 입력 시 동작은 플랫폼별 차이가 있을 수 있음
     */
    static constexpr FORCEINLINE float FloatSelect(float Comparand, float ValueGEZero, float ValueLTZero)
    {
        return Comparand >= 0.f ? ValueGEZero : ValueLTZero;
    }

    /**
     * 부동소수점 숫자가 거의 0에 가까운지 확인합니다.
     * @param Value 비교할 숫자
     * @param ErrorTolerance 거의 0으로 간주되는 값의 허용 최대 차이
     * @return Value가 거의 0에 가까우면 True
     */
    [[nodiscard]] static FORCEINLINE bool IsNearlyZero(float Value, float ErrorTolerance = SMALL_NUMBER)
    {
        return Abs<float>(Value) <= ErrorTolerance;
    }

    [[nodiscard]] static FORCEINLINE bool IsNearlyEqual(float A, float B, float ErrorTolerance = SMALL_NUMBER)
    {
        return Abs<float>(A - B) <= ErrorTolerance;
    }

    // Begin Interpolations
    /** A와 B를 Alpha값에 따라 선형으로 보간합니다. */
    template <typename T>
    [[nodiscard]] static FORCEINLINE constexpr T Lerp(const T& A, const T& B, float Alpha)
    {
        return static_cast<T>((A * (1.0f - Alpha)) + (B * Alpha));
    }

    /** A와 B를 Alpha값에 따라 선형으로 보간합니다. */
    template <typename T>
    [[nodiscard]] static FORCEINLINE constexpr T Lerp(const T& A, const T& B, double Alpha)
    {
        return static_cast<T>((A * (1.0 - Alpha)) + (B * Alpha));
    }

    /** TCustomLerp<T>에 있는 Lerp를 호출합니다. */
    template <typename T, typename U>
        requires (
            TCustomLerpable<T>
            && (std::is_floating_point_v<U> || std::is_same_v<T, U>)
            && !std::is_same_v<T, bool>
            && requires(const T& A, const T& B, const U& Alpha)
            {
                { TCustomLerp<T>::Lerp(A, B, Alpha) } -> std::same_as<T>;
            }
        )
    [[nodiscard]] static T Lerp(const T& A, const T& B, const U& Alpha)
    {
        return TCustomLerp<T>::Lerp(A, B, Alpha);
    }

    /** 서로 다른 타입에 대해 Lerp를 적용시킵니다. */
    template <typename T1, typename T2, typename T3>
        requires (
            !std::is_same_v<T1, T2>
            && !TCustomLerpable<T1>
            && !TCustomLerpable<T2>
            && requires(const T1& A, const T2& B, const T3& Alpha)
            {
                { Lerp(A, B, Alpha) } -> std::same_as<decltype(A * B)>;
            }
        )
    [[nodiscard]] static auto Lerp(const T1& A, const T2& B, const T3& Alpha) -> decltype(A * B)
    {
        using ABType = decltype(A * B);
        return Lerp(ABType(A), ABType(B), Alpha);
    }

    /**
     * 삼차 보간을 수행합니다.
     *
     * @param  P0, P1 - 끝점
     * @param  T0, T1 - 끝점에서의 접선 방향
     * @param  A - 스플라인을 따라 이동한 거리
     *
     * @return  보간된 값
     */
    template <typename T, typename U>
        requires (
            !TCustomLerpable<T>
            && (std::is_floating_point_v<U> || std::is_same_v<T, U>)
        )
    [[nodiscard]] static constexpr FORCEINLINE_DEBUGGABLE T CubicInterp(const T& P0, const T& T0, const T& P1, const T& T1, const U& A)
    {
        const U A2 = A * A;
        const U A3 = A2 * A;

        return T((((2*A3)-(3*A2)+1) * P0) + ((A3-(2*A2)+A) * T0) + ((A3-A2) * T1) + (((-2*A3)+(3*A2)) * P1));
    }

    /** 커스텀 타입에 대해서 CubicInterp을 수행합니다. */
    template <typename T, typename U>
        requires TCustomLerpable<T>
        && requires(const T& P0, const T& T0, const T& P1, const T& T1, const U& A)
        {
            { TCustomLerp<T>::CubicInterp(P0, T0, P1, T1, A) } -> std::same_as<T>;
        }
    [[nodiscard]] static FORCEINLINE_DEBUGGABLE T CubicInterp(const T& P0, const T& T0, const T& P1, const T& T1, const U& A)
    {
        return TCustomLerp<T>::CubicInterp(P0, T0, P1, T1, A);
    }

    /** A와 B 사이를 보간하며 ease in 함수를 적용합니다. Exp는 곡선의 강도를 조절합니다. */
    template <typename T>
    [[nodiscard]] static FORCEINLINE_DEBUGGABLE T InterpEaseIn(const T& A, const T& B, float Alpha, float Exp)
    {
        float const ModifiedAlpha = Pow(Alpha, Exp);
        return Lerp<T>(A, B, ModifiedAlpha);
    }

    /** A와 B 사이를 보간하며 ease out 함수를 적용합니다. Exp는 곡선의 강도를 조절합니다. */
    template <typename T>
    [[nodiscard]] static FORCEINLINE_DEBUGGABLE T InterpEaseOut(const T& A, const T& B, float Alpha, float Exp)
    {
        float const ModifiedAlpha = 1.f - Pow(1.f - Alpha, Exp);
        return Lerp<T>(A, B, ModifiedAlpha);
    }

    /** A와 B 사이를 보간하며 ease in/out 함수를 적용합니다. Exp는 곡선의 강도를 조절합니다. */
    template <typename T>
    [[nodiscard]] static FORCEINLINE_DEBUGGABLE T InterpEaseInOut(const T& A, const T& B, float Alpha, float Exp)
    {
        return Lerp<T>(
            A, B, (Alpha < 0.5f)
                ? InterpEaseIn(0.f, 1.f, Alpha * 2.f, Exp) * 0.5f
                : InterpEaseOut(0.f, 1.f, Alpha * 2.f - 1.f, Exp) * 0.5f + 0.5f
        );
    }

    /** A와 B 사이를 step 함수 방식으로 보간합니다. */
    template <typename T>
    [[nodiscard]] static constexpr FORCEINLINE_DEBUGGABLE T InterpStep(const T& A, const T& B, float Alpha, int32 Steps)
    {
        if (Steps <= 1 || Alpha <= 0)
        {
            return A;
        }
        else if (Alpha >= 1)
        {
            return B;
        }

        const float StepsAsFloat = static_cast<float>(Steps);
        const float NumIntervals = StepsAsFloat - 1.f;
        float const ModifiedAlpha = FloorToFloat(Alpha * StepsAsFloat) / NumIntervals;
        return Lerp<T>(A, B, ModifiedAlpha);
    }

    /** A와 B 사이를 sinusoidal in 함수 방식으로 보간합니다. */
    template <typename T>
    [[nodiscard]] static FORCEINLINE_DEBUGGABLE T InterpSinIn(const T& A, const T& B, float Alpha)
    {
        float const ModifiedAlpha = -1.f * Cos(Alpha * HALF_PI) + 1.f;
        return Lerp<T>(A, B, ModifiedAlpha);
    }

    /** A와 B 사이를 sinusoidal out 함수 방식으로 보간합니다. */
    template <typename T>
    [[nodiscard]] static FORCEINLINE_DEBUGGABLE T InterpSinOut(const T& A, const T& B, float Alpha)
    {
        float const ModifiedAlpha = Sin(Alpha * HALF_PI);
        return Lerp<T>(A, B, ModifiedAlpha);
    }

    /** A와 B 사이를 sinusoidal in/out 함수 방식으로 보간합니다. */
    template <typename T>
    [[nodiscard]] static FORCEINLINE_DEBUGGABLE T InterpSinInOut(const T& A, const T& B, float Alpha)
    {
        return Lerp<T>(A, B, (Alpha < 0.5f)
            ? InterpSinIn(0.f, 1.f, Alpha * 2.f) * 0.5f
            : InterpSinOut(0.f, 1.f, Alpha * 2.f - 1.f) * 0.5f + 0.5f
        );
    }

    /** A와 B 사이를 exponential in 함수 방식으로 보간합니다. */
    template <typename T>
    [[nodiscard]] static FORCEINLINE_DEBUGGABLE T InterpExpoIn(const T& A, const T& B, float Alpha)
    {
        float const ModifiedAlpha = (Alpha == 0.f) ? 0.f : Pow(2.f, 10.f * (Alpha - 1.f));
        return Lerp<T>(A, B, ModifiedAlpha);
    }

    /** A와 B 사이를 exponential out 함수 방식으로 보간합니다. */
    template <typename T>
    [[nodiscard]] static FORCEINLINE_DEBUGGABLE T InterpExpoOut(const T& A, const T& B, float Alpha)
    {
        float const ModifiedAlpha = (Alpha == 1.f) ? 1.f : -Pow(2.f, -10.f * Alpha) + 1.f;
        return Lerp<T>(A, B, ModifiedAlpha);
    }

    /** A와 B 사이를 exponential in/out 함수 방식으로 보간합니다. */
    template <typename T>
    [[nodiscard]] static FORCEINLINE_DEBUGGABLE T InterpExpoInOut(const T& A, const T& B, float Alpha)
    {
        return Lerp<T>(A, B, (Alpha < 0.5f)
            ? InterpExpoIn(0.f, 1.f, Alpha * 2.f) * 0.5f
            : InterpExpoOut(0.f, 1.f, Alpha * 2.f - 1.f) * 0.5f + 0.5f
        );
    }

    /** A와 B 사이를 circular in 함수 방식으로 보간합니다. */
    template <typename T>
    [[nodiscard]] static FORCEINLINE_DEBUGGABLE T InterpCircularIn(const T& A, const T& B, float Alpha)
    {
        float const ModifiedAlpha = -1.f * (Sqrt(1.f - Alpha * Alpha) - 1.f);
        return Lerp<T>(A, B, ModifiedAlpha);
    }

    /** A와 B 사이를 circular out 함수 방식으로 보간합니다. */
    template <typename T>
    [[nodiscard]] static FORCEINLINE_DEBUGGABLE T InterpCircularOut(const T& A, const T& B, float Alpha)
    {
        Alpha -= 1.f;
        float const ModifiedAlpha = Sqrt(1.f - Alpha * Alpha);
        return Lerp<T>(A, B, ModifiedAlpha);
    }

    /** A와 B 사이를 circular in/out 함수 방식으로 보간합니다. */
    template <typename T>
    [[nodiscard]] static FORCEINLINE_DEBUGGABLE T InterpCircularInOut(const T& A, const T& B, float Alpha)
    {
        return Lerp<T>(
            A, B, (Alpha < 0.5f)
                ? InterpCircularIn(0.f, 1.f, Alpha * 2.f) * 0.5f
                : InterpCircularOut(0.f, 1.f, Alpha * 2.f - 1.f) * 0.5f + 0.5f
        );
    }

    // Special-case interpolation
    /** Current와 Target 법선 벡터를 일정 각도 단계로 보간합니다. */
    [[nodiscard]] static FVector VInterpNormalRotationTo(const FVector& Current, const FVector& Target, float DeltaTime, float RotationSpeedDegrees);

    /** Current에서 Target까지 일정 속도로 벡터 보간 */
    [[nodiscard]] static FVector VInterpConstantTo(const FVector& Current, const FVector& Target, float DeltaTime, float InterpSpeed);

    /** Current에서 Target까지 벡터 보간 (타겟까지 거리에 비례하여 초기 고속 적용 후 감속) */
    [[nodiscard]] static FVector VInterpTo(const FVector& Current, const FVector& Target, float DeltaTime, float InterpSpeed);

    /** Current에서 Target까지 2D 벡터를 일정 속도로 보간 */
    [[nodiscard]] static FVector2D Vector2DInterpConstantTo(const FVector2D& Current, const FVector2D& Target, float DeltaTime, float InterpSpeed);

    /** Current에서 Target까지 2D 벡터 보간 (타겟까지 거리에 비례하여 초기 고속 적용 후 감속) */
    [[nodiscard]] static FVector2D Vector2DInterpTo(const FVector2D& Current, const FVector2D& Target, float DeltaTime, float InterpSpeed);

    /** Current에서 Target까지 회전체를 일정 속도로 보간 */
    [[nodiscard]] static FRotator RInterpConstantTo(const FRotator& Current, const FRotator& Target, float DeltaTime, float InterpSpeed);

    /** Current에서 Target까지 회전체 보간 (타겟까지 거리에 비례하여 초기 고속 적용 후 감속) */
    [[nodiscard]] static FRotator RInterpTo(const FRotator& Current, const FRotator& Target, float DeltaTime, float InterpSpeed);

    /** Current에서 Target까지 부동소수점 값 일정 속도 보간 */
    template <typename T1, typename T2 = T1, typename T3 = T2, typename T4 = T3>
        requires (!std::is_same_v<T1, bool> && !std::is_same_v<T2, bool>)
    [[nodiscard]] static auto FInterpConstantTo(T1 Current, T2 Target, T3 DeltaTime, T4 InterpSpeed)
    {
        using RetType = decltype(T1() * T2() * T3() * T4());

        const RetType Dist = Target - Current;

        // If distance is too small, just set the desired location
        if (FMath::Square(Dist) < SMALL_NUMBER)
        {
            return static_cast<RetType>(Target);
        }

        const RetType Step = InterpSpeed * DeltaTime;
        return Current + FMath::Clamp(Dist, -Step, Step);
    }

    /** Current에서 Target까지 부동소수점 값 보간 (타겟까지 거리에 비례하여 초기 고속 적용 후 감속) */
    template <typename T1, typename T2 = T1, typename T3 = T2, typename T4 = T3>
        requires (!std::is_same_v<T1, bool> && !std::is_same_v<T2, bool>)
    [[nodiscard]] static auto FInterpTo(T1 Current, T2 Target, T3 DeltaTime, T4 InterpSpeed)
    {
        using RetType = decltype(T1() * T2() * T3() * T4());

        // If no interp speed, jump to target value
        if (InterpSpeed <= 0.f)
        {
            return static_cast<RetType>(Target);
        }

        // Distance to reach
        const RetType Dist = Target - Current;

        // If distance is too small, just set the desired location
        if (FMath::Square(Dist) < SMALL_NUMBER)
        {
            return static_cast<RetType>(Target);
        }

        // Delta Move, Clamp so we do not over shoot.
        const RetType DeltaMove = Dist * FMath::Clamp<RetType>(DeltaTime * InterpSpeed, 0.f, 1.f);
        return Current + DeltaMove;
    }

    /** Current에서 Target까지 선형 색상 보간 (타겟까지 거리에 비례하여 초기 고속 적용 후 감속) */
    [[nodiscard]] static FLinearColor CInterpTo(const FLinearColor& Current, const FLinearColor& Target, float DeltaTime, float InterpSpeed);

    /** Current에서 Target까지 쿼터니언을 일정 각속도(라디안)로 보간 */
    [[nodiscard]] static FQuat QInterpConstantTo(const FQuat& Current, const FQuat& Target, float DeltaTime, float InterpSpeed);

    /** Current에서 Target까지 쿼터니언 보간 (타겟 각도 차이에 비례하여 초기 고속 적용 후 감속) */
    [[nodiscard]] static FQuat QInterpTo(const FQuat& Current, const FQuat& Target, float DeltaTime, float InterpSpeed);

    // 다른 Lerp는 UnrealMathUtility.h의 1469
    // End Interpolations


    template <typename T>
    [[nodiscard]] static FORCEINLINE constexpr auto RadiansToDegrees(const T& RadVal) -> decltype(RadVal * (180.0f / PI))
    {
        return RadVal * (180.0f / PI);
    }

    [[nodiscard]] static FORCEINLINE constexpr float RadiansToDegrees(float RadVal)
    {
        return RadVal * (180.0f / PI);
    }

    [[nodiscard]] static FORCEINLINE constexpr double RadiansToDegrees(double RadVal)
    {
        return RadVal * (180.0 / PI_DOUBLE);
    }

    template <typename T>
    [[nodiscard]] static FORCEINLINE constexpr auto DegreesToRadians(const T& DegVal) -> decltype(DegVal * (PI / 180.0f))
    {
        return DegVal * (PI / 180.0f);
    }

    [[nodiscard]] static FORCEINLINE constexpr float DegreesToRadians(float DegVal)
    {
        return DegVal * (PI / 180.0f);
    }

    [[nodiscard]] static FORCEINLINE constexpr double DegreesToRadians(double DegVal)
    {
        return DegVal * (PI_DOUBLE / 180.0);
    }

    // Returns e^Value
    [[nodiscard]] static FORCEINLINE float Exp(float Value) { return expf(Value); }
    [[nodiscard]] static FORCEINLINE double Exp(double Value) { return exp(Value); }

    // Returns 2^Value
    [[nodiscard]] static FORCEINLINE float Exp2(float Value) { return powf(2.f, Value); /*exp2f(Value);*/ }
    [[nodiscard]] static FORCEINLINE double Exp2(double Value) {return pow(2.0, Value); /*exp2(Value);*/ }

    [[nodiscard]] static FORCEINLINE float Loge(float Value) { return logf(Value); }
    [[nodiscard]] static FORCEINLINE double Loge(double Value) { return log(Value); }

    [[nodiscard]] static FORCEINLINE float LogX(float Base, float Value) { return Loge(Value) / Loge(Base); }
    [[nodiscard]] static FORCEINLINE double LogX(double Base, double Value) { return Loge(Value) / Loge(Base); }

    // 1.0 / Loge(2) = 1.4426950f
    [[nodiscard]] static FORCEINLINE float Log2(float Value) { return Loge(Value) * std::numbers::log2e_v<float>; }
    // 1.0 / Loge(2) = 1.442695040888963387
    [[nodiscard]] static FORCEINLINE double Log2(double Value) { return Loge(Value) * std::numbers::log2e; }


    [[nodiscard]] static FORCEINLINE double Cos(double RadVal) { return cos(RadVal); }
    [[nodiscard]] static FORCEINLINE float Cos(float RadVal) { return cosf(RadVal); }

    [[nodiscard]] static FORCEINLINE double Sin(double RadVal) { return sin(RadVal); }
    [[nodiscard]] static FORCEINLINE float Sin(float RadVal) { return sinf(RadVal); }

    [[nodiscard]] static FORCEINLINE double Tan(double RadVal) { return tan(RadVal); }
    [[nodiscard]] static FORCEINLINE float Tan(float RadVal) { return tanf(RadVal); }

    [[nodiscard]] static FORCEINLINE double Acos(double Value) { return acos(Value); }
    [[nodiscard]] static FORCEINLINE float Acos(float Value) { return acosf(Value); }

    [[nodiscard]] static FORCEINLINE double Asin(double Value) { return asin(Value); }
    [[nodiscard]] static FORCEINLINE float Asin(float Value) { return asinf(Value); }

    [[nodiscard]] static FORCEINLINE double Atan(double Value) { return atan(Value); }
    [[nodiscard]] static FORCEINLINE float Atan(float Value) { return atanf(Value); }

    [[nodiscard]] static FORCEINLINE double Atan2(double Y, double X) { return atan2(Y, X); }
    [[nodiscard]] static FORCEINLINE float Atan2(float Y, float X) { return atan2f(Y, X); }

    static FORCEINLINE void SinCos(float* ScalarSin, float* ScalarCos, float Value)
    {
        *ScalarSin = sinf(Value);
        *ScalarCos = cosf(Value);
    }

    static FORCEINLINE void SinCos(double* ScalarSin, double* ScalarCos, double Value)
    {
        *ScalarSin = sin(Value);
        *ScalarCos = cos(Value);
    }

    [[nodiscard]] static FORCEINLINE int32 CeilToInt(float Value) { return static_cast<int32>(ceilf(Value)); }
    [[nodiscard]] static FORCEINLINE int32 CeilToInt(double Value) { return static_cast<int32>(ceil(Value)); }

    template <typename T>
    [[nodiscard]] static FORCEINLINE int32 CeilToInt(T Value) { return static_cast<int32>(ceil(Value)); }

    [[nodiscard]] static FORCEINLINE float TruncToFloat(float F) { return SSE::TruncToFloat(F); }
    [[nodiscard]] static FORCEINLINE double TruncToDouble(double F) { return SSE::TruncToDouble(F); }

    [[nodiscard]] static FORCEINLINE float FloorToFloat(float F) { return SSE::FloorToFloat(F); }
    [[nodiscard]] static FORCEINLINE double FloorToDouble(double F) { return SSE::FloorToDouble(F); }

    [[nodiscard]] static FORCEINLINE float RoundToFloat(float F) { return FloorToFloat(F + 0.5f); }
    [[nodiscard]] static FORCEINLINE double RoundToDouble(double F) { return FloorToDouble(F + 0.5); }

    [[nodiscard]] static FORCEINLINE float CeilToFloat(float F) { return SSE::CeilToFloat(F); }
    [[nodiscard]] static FORCEINLINE double CeilToDouble(double F) { return SSE::CeilToDouble(F); }

    [[nodiscard]] static FORCEINLINE float UnwindDegrees(float A)
    {
        A = FMath::Fmod(A + 180.0f, 360.0f);
        if (A < 0)
            A += 360.0f;
        return A - 180.0f;
    }

    [[nodiscard]] static float Fmod(float X, float Y)
    {
        const float AbsY = FMath::Abs(Y);
        if (AbsY <= SMALL_NUMBER)
        {
            return 0.0;
        }

        return fmodf(X, Y);
    }

    [[nodiscard]]static int RandHelper(int max)
    {
        static std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> dist(0, max - 1);
        return dist(rng);
    }

    [[nodiscard]] static float PerlinNoise1D(float x)
    {
        static auto fade = [](float t) -> float
        {
            return t * t * t * (t * (t * 6 - 15) + 10);
        };

        static auto lerp = [](float a, float b, float t) -> float
        {
            return a + t * (b - a);
        };

        static auto grad = [](int hash, float x) -> float
        {
            int h = hash & 15;
            float grad = 1.0f + (h & 7); // Gradient value 1-8
            if (h & 8) grad = -grad;
            return grad * x;
        };

        static int p[512] = {
            151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,
            151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
        };

        int xi = static_cast<int>(std::floor(x)) & 255;
        float xf = x - std::floor(x);
        float u = fade(xf);
        int a = p[xi];
        int b = p[xi + 1];
        return lerp(grad(a, xf), grad(b, xf - 1.0f), u);
    }

    template <typename T>
    [[nodiscard]] static T Min3(const T& A, const T& B, const T& C)
    {
        return FMath::Min(A, FMath::Min(B, C));
    }

    static FORCEINLINE bool IsFinite(float x) { return _finite(x); }
    static FORCEINLINE bool IsInf(float x) { return !_finite(x); }
    static FORCEINLINE bool IsNaN(float A) { return _isnan(A); }

    static FORCEINLINE int32 TruncToInt(float F) { return static_cast<int32>(std::trunc(F)); }
    static FORCEINLINE int64 TruncToInt(double F) { return static_cast<int64>(std::trunc(F)); }
    static FORCEINLINE int32 FloorToInt(float F) { return static_cast<int32>(std::floor(F)); }
    static FORCEINLINE int64 FloorToInt(double F) { return static_cast<int64>(std::floor(F)); }
    static FORCEINLINE int32 RoundToInt(float F) { return static_cast<int32>(std::round(F)); }
    static FORCEINLINE int64 RoundToInt(double F) { return static_cast<int64>(std::round(F)); }
    
    static FORCEINLINE float Frac(float Value)
    {
        return Value - FloorToFloat(Value);
    }

    static FORCEINLINE double Frac(double Value)
    {
        return Value - FloorToDouble(Value);
    }
};
