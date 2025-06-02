#include "InputComponent.h"
#include "Runtime/Windows/WindowsCursor.h"

UInputComponent::UInputComponent()
{
    MouseX = FWindowsCursor::GetPosition().X;
    MouseY = FWindowsCursor::GetPosition().Y;
}

void UInputComponent::ProcessInput(float DeltaTime)
{ 
    if (!bShowCursor)
    {
        for (EKeys::Type Key : PressedKeys)
        {
            if (KeyBindDelegate.Contains(Key))
            {
                KeyBindDelegate[Key].Broadcast(DeltaTime);
            }
            else if (MouseButtonBindDelegate.Contains(EKeys::ToMouseButton(Key)))
            {
                MouseButtonBindDelegate[EKeys::ToMouseButton(Key)].Broadcast(DeltaTime);
            }
        }

        MouseMoveBindDelegate.Broadcast(MouseDeltaX, MouseDeltaY);
    }

    if (PressedKeys.Contains(EKeys::Escape))
    {
        PressedKeys.Remove(EKeys::Escape);
        bShowCursor = !bShowCursor;
        if (bShowCursor)
        {
            FWindowsCursor::SetPosition(MouseX, MouseY);
        }
        else
        {
            MouseX = FWindowsCursor::GetPosition().X;
            MouseY = FWindowsCursor::GetPosition().Y;
        }
    }
    
    FWindowsCursor::SetShowMouseCursor(bShowCursor);
    
    if (!bShowCursor)
    {
        FWindowsCursor::SetPosition(MouseX, MouseY);
    }
    MouseDeltaX = 0.f;
    MouseDeltaY = 0.f;
    return;
}

void UInputComponent::SetPossess()
{
    PressedKeys.Empty();
    BindInputDelegate();

    MouseDeltaX = 0.f;
    MouseDeltaY = 0.f;
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

    BindMouseMoveDelegateHandles.Add(Handler->OnRawMouseInputDelegate.AddLambda([this](const FPointerEvent& InMouseEvent)
    {
        InputMouseMove(InMouseEvent);
    }));

    BindMouseDownDelegateHandles.Add(Handler->OnRawMouseInputDelegate.AddLambda([this](const FPointerEvent& InMouseEvent)
    {
        InputMouseButton(InMouseEvent);
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

    for (FDelegateHandle DelegateHandle : BindMouseMoveDelegateHandles)
    {
        Handler->OnRawMouseInputDelegate.Remove(DelegateHandle);
    }

    for (FDelegateHandle DelegateHandle : BindMouseDownDelegateHandles)
    {
        Handler->OnRawMouseInputDelegate.Remove(DelegateHandle);
    }

    //for (FDelegateHandle DelegateHandle : BindMouseUpDelegateHandles)
    //{
    //    Handler->OnMouseUpDelegate.Remove(DelegateHandle);
    //}

    BindKeyDownDelegateHandles.Empty();
    BindKeyUpDelegateHandles.Empty();
    BindMouseMoveDelegateHandles.Empty();
    BindMouseDownDelegateHandles.Empty();
    //BindMouseUpDelegateHandles.Empty();
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
    MouseDeltaX = Delta.X;
    MouseDeltaY = Delta.Y;       
}

void UInputComponent::InputMouseButton(const FPointerEvent& InMouseEvent)
{
    EKeys::Type EffectingButton = InMouseEvent.GetEffectingButton();
    if (InMouseEvent.GetInputEvent() == IE_Pressed)
    {
        if (!PressedKeys.Contains(EffectingButton))
        {
            PressedKeys.Add(EffectingButton);
        }
    }
    else if (InMouseEvent.GetInputEvent() == IE_Released)
    {
        if (PressedKeys.Contains(EffectingButton))
        {
            PressedKeys.Remove(EffectingButton);
        }
    }
}
