#pragma once
#include <type_traits>
#include "Core/Container/Map.h"
#include "Delegates/DelegateCombination.h"
#include "Runtime/InputCore/InputCoreTypes.h"
#include "Components/ActorComponent.h"

//DECLARE_MULTICAST_DELEGATE(FVoidDelegate)
DECLARE_MULTICAST_DELEGATE_OneParam(FOneFloatDelegate, const float&)
DECLARE_MULTICAST_DELEGATE_TwoParams(FTwoFloatDelegate, const float&, const float&)


class UInputComponent : public UActorComponent
{
    DECLARE_CLASS(UInputComponent, UActorComponent)

public:
    UInputComponent();
    virtual ~UInputComponent() override = default;

    template<typename Func>
    void BindKey(EKeys::Type Key, Func&& Callback);

    template<typename Func>
    inline std::enable_if_t<
        std::is_invocable_v<Func, float, float>,
        void
    >
    BindMouseMove(Func&& Callback);

    template<typename Func>
    void BindMouseDown(EMouseButtons::Type Button, Func&& Callback);

    void ProcessInput(float DeltaTime);
    
    void SetPossess();
    void BindInputDelegate();
    void UnPossess();
    void ClearBindDelegate();
    // Possess가 풀렸다가 다시 왔을때 원래 바인딩 돼있던 애들 일괄적으로 다시 바인딩해줘야할수도 있음.
    void InputKey(const FKeyEvent& InKeyEvent);
    void InputMouseMove(const FPointerEvent& InMouseEvent);
    void InputMouseButton(const FPointerEvent& InMouseEvent);

private:
    TArray<FDelegateHandle> BindKeyDownDelegateHandles;
    TArray<FDelegateHandle> BindKeyUpDelegateHandles;
    TArray<FDelegateHandle> BindMouseDownDelegateHandles;
    TArray<FDelegateHandle> BindMouseUpDelegateHandles;
    TArray<FDelegateHandle> BindMouseMoveDelegateHandles;
    TMap<EKeys::Type, FOneFloatDelegate> KeyBindDelegate;
    TMap<EMouseButtons::Type, FOneFloatDelegate> MouseButtonBindDelegate;
    FTwoFloatDelegate MouseMoveBindDelegate;

    TSet<EKeys::Type> PressedKeys;
    //TSet<EMouseButtons::Type> PressedMouseButtons;
    float MouseDeltaX = 0.0f;
    float MouseDeltaY = 0.0f;

    float MouseX = 0.0f; // 현재 마우스 X 위치
    float MouseY = 0.0f; // 현재 마우스 Y 위치

    bool bShowCursor = false;
};

template<typename Func>
inline void UInputComponent::BindKey(EKeys::Type Key, Func&& Callback)
{
    if (Callback == nullptr)
    {
        return;
    }
    // 버튼 입력만 처리
    if (Key != EKeys::AnyKey && Key <= EKeys::MouseWheelAxis || Key >= EKeys::Gamepad_Left2D)
    {
        assert(false && "Only keyboard keys are supported for key binding.");
        return;
    }
    KeyBindDelegate[Key].AddLambda(std::forward<Func>(Callback));
}

// 마우스 이동에 대한 콜백 함수를 등록합니다.
// 첫 두 인자는 마우스의 deltaX, deltaY 입니다.
template<typename Func>
inline std::enable_if_t<
    std::is_invocable_v<Func, float, float>,
    void
>
UInputComponent::BindMouseMove(Func&& Callback)
{
    if (Callback == nullptr)
    {
        return;
    }
    MouseMoveBindDelegate.AddLambda(std::forward<Func>(Callback));
}

template<typename Func>
inline void UInputComponent::BindMouseDown(EMouseButtons::Type Button, Func&& Callback)
{
    if (Callback == nullptr)
    {
        return;
    }
    MouseButtonBindDelegate[Button].AddLambda(std::forward<Func>(Callback));
}
