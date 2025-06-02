#pragma once

#include "LuaUI.h"

class LuaButtonUI : public LuaUI 
{
public:
    LuaButtonUI(FName InName);
    LuaButtonUI(FName InName, RectTransform InRectTransform, int InSortOrder, FString InLuaFunctionName);

    virtual void DrawImGuiUI() override;


private:
    void ButtonDownEvent();
    void ButtonEvent();
    void ButtonUpEvent();

    void HoverInEvent();
    void HoverEvent();
    void HoverOutEvent();

    bool bCurKeyStateDown;
    bool bCurHoverStateIn;
    FString LuaFunctionName;
};
