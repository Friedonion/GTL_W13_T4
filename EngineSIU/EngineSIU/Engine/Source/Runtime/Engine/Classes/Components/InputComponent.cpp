#include "InputComponent.h"

void UInputComponent::ProcessInput(float DeltaTime)
{
    for (EKeys::Type Key : PressedKeys)
    {
        FString KeyName = EKeys::ToString(Key);
        if (KeyBindDelegate.Contains(KeyName))
        {
            KeyBindDelegate[KeyName].Broadcast(DeltaTime);
        }
    }
    return;
    //if (PressedKeys.Contains(EKeys::W))
    //{
    //    KeyBindDelegate[FString("W")].Broadcast(DeltaTime);
    //}
    //if (PressedKeys.Contains(EKeys::A))
    //{
    //    KeyBindDelegate[FString("A")].Broadcast(DeltaTime);
    //}
    //if (PressedKeys.Contains(EKeys::S))
    //{
    //    KeyBindDelegate[FString("S")].Broadcast(DeltaTime);
    //}
    //if (PressedKeys.Contains(EKeys::D))
    //{
    //    KeyBindDelegate[FString("D")].Broadcast(DeltaTime);
    //}
    //if (PressedKeys.Contains(EKeys::SpaceBar))
    //{
    //    KeyBindDelegate[FString("Space")].Broadcast(DeltaTime);
    //}

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

    //// 일반적인 단일 키 이벤트
    //switch (InKeyEvent.GetCharacter())
    //{
    //case 'W':
    //    {
    //        if (InKeyEvent.GetInputEvent() == IE_Pressed)
    //        {
    //            PressedKeys.Add(EKeys::W);
    //        }
    //        else if (InKeyEvent.GetInputEvent() == IE_Released)
    //        {
    //            PressedKeys.Remove(EKeys::W);
    //        }
    //        break;
    //    }
    //case 'A':
    //    {
    //        if (InKeyEvent.GetInputEvent() == IE_Pressed)
    //        {
    //            PressedKeys.Add(EKeys::A);
    //        }
    //        else if (InKeyEvent.GetInputEvent() == IE_Released)
    //        {
    //            PressedKeys.Remove(EKeys::A);
    //        }
    //        break;
    //    }
    //case 'S':
    //    {
    //        if (InKeyEvent.GetInputEvent() == IE_Pressed)
    //        {
    //            PressedKeys.Add(EKeys::S);
    //        }
    //        else if (InKeyEvent.GetInputEvent() == IE_Released)
    //        {
    //            PressedKeys.Remove(EKeys::S);
    //        }
    //        break;
    //    }
    //case 'D':
    //    {
    //        if (InKeyEvent.GetInputEvent() == IE_Pressed)
    //        {
    //            PressedKeys.Add(EKeys::D);
    //        }
    //        else if (InKeyEvent.GetInputEvent() == IE_Released)
    //        {
    //            PressedKeys.Remove(EKeys::D);
    //        }
    //        break;
    //    }
    //default:
    //    break;
    //}
}


void UInputComponent::BindAction(const FString& Key, const std::function<void(float)>& Callback)
{
    if (Callback == nullptr)
    {
        return;
    }
    
    KeyBindDelegate[Key].AddLambda([this, Callback](float DeltaTime)
    {
        Callback(DeltaTime);
    });
}
