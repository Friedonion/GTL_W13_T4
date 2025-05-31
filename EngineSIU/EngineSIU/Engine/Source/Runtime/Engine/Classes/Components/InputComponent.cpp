#include "InputComponent.h"

void UInputComponent::ProcessInput(float DeltaTime)
{
    for (EKeys::Type Key : PressedKeys)
    {
        if (KeyBindDelegate.Contains(Key))
        {
            KeyBindDelegate[Key].Broadcast(DeltaTime);
        }
    }

    MouseBindDelegate.Broadcast(MouseX, MouseY);

    MouseX = 0.f;
    MouseY = 0.f;
    return;
}

void UInputComponent::SetPossess()
{
    BindInputDelegate();
    
    //TODO: Possess일때 기존에 있던거 다시 넣어줘야할수도
}

void UInputComponent::BindInputDelegate()
{
    FSlateAppMessageHandler* Handler = GEngineLoop.GetAppMessageHandler();

    BindKeyDownDelegateHandles.Add(Handler->OnKeyDownDelegate.AddLambda([this](const FKeyEvent& InKeyEvent)
    {
        InputKey(InKeyEvent);
    }));

    BindKeyUpDelegateHandles.Add(Handler->OnKeyUpDelegate.AddLambda([this](const FKeyEvent& InKeyEvent)
    {
        InputKey(InKeyEvent);
    }));

    BindMouseMoveDelegateHandles.Add(Handler->OnMouseMoveDelegate.AddLambda([this](const FPointerEvent& InMouseEvent)
    {
        InputMouseMove(InMouseEvent);
    }));
    
}

void UInputComponent::UnPossess()
{ 
    ClearBindDelegate();
}

void UInputComponent::ClearBindDelegate()
{
    FSlateAppMessageHandler* Handler = GEngineLoop.GetAppMessageHandler();

    for (FDelegateHandle DelegateHandle : BindKeyDownDelegateHandles)
    {
        Handler->OnKeyDownDelegate.Remove(DelegateHandle);
    }
     
    for (FDelegateHandle DelegateHandle : BindKeyUpDelegateHandles)
    {
        Handler->OnKeyUpDelegate.Remove(DelegateHandle);
    }
    
    BindKeyDownDelegateHandles.Empty();
    BindKeyUpDelegateHandles.Empty();
}

void UInputComponent::InputKey(const FKeyEvent& InKeyEvent)
{
    EKeys::Type Key = EKeys::FromCharCode(InKeyEvent.GetCharacter());
    if (InKeyEvent.GetInputEvent() == IE_Pressed)
    {
        if (!PressedKeys.Contains(Key))
        {
            PressedKeys.Add(Key);
            return;
        }
    }
    else if (InKeyEvent.GetInputEvent() == IE_Released)
    {
        if (PressedKeys.Contains(Key))
        {
            PressedKeys.Remove(Key);
        }
    }
    return;
}

void UInputComponent::InputMouseMove(const FPointerEvent& InMouseEvent)
{
    FVector2D Delta = InMouseEvent.GetCursorDelta();
    MouseX = Delta.X;
    MouseY = Delta.Y;
}

// 해당 키에 대한 콜백 함수를 바인딩합니다.
// 사용법
/*
// C++ 코드
AActor::BeginPlay()
{
    ...
    sol::state Lua; // 파라미터로 받은거
    Lua.set_function("controller",
        [](const std::string& Key, const std::function<void(float)>& Callback)
        {
            GEngine->ActiveWorld->GetPlayerController()->BindAction(FString(Key), Callback);
        }
    );

    CallLuaFunction("InitializeLua");
    ...
}

// Lua 코드
function InitializeLua()
    controller("W", OnPressW)
    controller("S", OnPressS)
    controller("A", OnPressA)
    controller("D", OnPressD)
end

function OnPressW(DeltaTime)
    print("W key pressed. DeltaTime: " .. DeltaTime)
end
*/

