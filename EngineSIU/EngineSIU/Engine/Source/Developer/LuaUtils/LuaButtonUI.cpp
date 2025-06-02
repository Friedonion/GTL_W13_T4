#include "LuaButtonUI.h"
#include "Engine/Source/ThirdParty/ImGui/include/ImGui/imgui.h"
#include "Engine/UserInterface/Console.h"

LuaButtonUI::LuaButtonUI(FName InName)
    :LuaUI(InName)
{
}

LuaButtonUI::LuaButtonUI(FName InName, RectTransform InRectTransform, int InSortOrder, FString InLuaFunctionName)
    :LuaUI(InName)
{
    Rect = InRectTransform;
    SortOrder = InSortOrder;
    bCurKeyStateDown = false;
    bCurHoverStateIn = false;
    LuaFunctionName = InLuaFunctionName;
}

void LuaButtonUI::DrawImGuiUI()
{
    if (!GetVisible())
        return;

    RectTransform worldRect = GetWorldRectTransform();
    ImVec2 screenPos = ImVec2(worldRect.Position.X, worldRect.Position.Y);
    ImVec2 size = ImVec2(worldRect.Size.X, worldRect.Size.Y);

    ImGui::SetCursorScreenPos(screenPos);


    bool isClicked = ImGui::InvisibleButton(*GetName().ToString(), size);

    bool isHovered = ImGui::IsItemHovered();
    bool isMouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);

    if (isClicked && !bCurKeyStateDown)
    {
        bCurKeyStateDown = true;
        ButtonDownEvent();
    }

    if (isClicked)
    {
        ButtonEvent();
    }

    if (bCurKeyStateDown && !isClicked)
    {
        bCurKeyStateDown = false;
        ButtonUpEvent();
    }

    if (isHovered) 
    {
        HoverEvent();
    }

    if (!bCurHoverStateIn && isHovered) 
    {
        HoverInEvent();
    }

    if (bCurHoverStateIn && !isHovered) 
    {
        HoverOutEvent();
    }

    bCurHoverStateIn = isHovered;
}

void LuaButtonUI::ButtonDownEvent()
{
    // TODO Name을 기반으로 Lua 쪽과 연결
    UE_LOG(ELogLevel::Display, "ButtonDownEvent");
}

void LuaButtonUI::ButtonEvent()
{
    // TODO Name을 기반으로 Lua 쪽과 연결
    UE_LOG(ELogLevel::Display, "ButtonEvent");
}

void LuaButtonUI::ButtonUpEvent()
{
    // TODO Name을 기반으로 Lua 쪽과 연결
    UE_LOG(ELogLevel::Display, "ButtonUpEvent");
}

void LuaButtonUI::HoverInEvent()
{
    UE_LOG(ELogLevel::Display, "HoverInEvent");
}

void LuaButtonUI::HoverEvent()
{
    UE_LOG(ELogLevel::Display, "HoverEvent");
}

void LuaButtonUI::HoverOutEvent()
{
    UE_LOG(ELogLevel::Display, "HoverOutEvent");
}
