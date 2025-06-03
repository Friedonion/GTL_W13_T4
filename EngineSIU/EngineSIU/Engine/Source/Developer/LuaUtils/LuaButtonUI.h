#pragma once

#include "LuaUI.h"
#include <functional>
#include "Engine/Source/ThirdParty/ImGui/include/ImGui/imgui.h"
class LuaButtonUI : public LuaUI
{
public:
    LuaButtonUI(FName InName);
    LuaButtonUI(FName InName, RectTransform InRectTransform, int InSortOrder, FString InLuaFunctionName);

    virtual void DrawImGuiUI() override;

    void SetTexture(ImTextureID InTextureID);

    void SetOnClick(std::function<void()> InCallback);
    void SetOnHoverIn(std::function<void()> InCallback);
    void SetOnHoverOut(std::function<void()> InCallback);

private:
    void ButtonDownEvent();
    void ButtonEvent();
    void ButtonUpEvent();
    void HoverInEvent();
    void HoverEvent();
    void HoverOutEvent();

    bool bCurKeyStateDown = false;
    bool bCurHoverStateIn = false;

    FString LuaFunctionName;
    ImTextureID TextureID;

    std::function<void()> OnClick;
    std::function<void()> OnHoverIn;
    std::function<void()> OnHoverOut;
};
