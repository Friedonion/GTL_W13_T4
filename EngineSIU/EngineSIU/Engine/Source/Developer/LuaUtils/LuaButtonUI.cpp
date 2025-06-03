#include "LuaButtonUI.h"
#include "Engine/Source/ThirdParty/ImGui/include/ImGui/imgui.h"
#include "Engine/UserInterface/Console.h"
#include "Lua/LuaScriptManager.h"
#include "Lua/LuaScriptComponent.h"

LuaButtonUI::LuaButtonUI(FName InName)
    : LuaUI(InName)
{
}

LuaButtonUI::LuaButtonUI(FName InName, RectTransform InRectTransform, int InSortOrder, FString InLuaFunctionName)
    : LuaUI(InName), LuaFunctionName(InLuaFunctionName)
{
    Rect = InRectTransform;
    SortOrder = InSortOrder;
    Visible = true;
}

void LuaButtonUI::SetTexture(ImTextureID InTextureID)
{
    TextureID = InTextureID;
}

void LuaButtonUI::SetOnClick(std::function<void()> InCallback)
{
    OnClick = InCallback;
}

void LuaButtonUI::SetOnHoverIn(std::function<void()> InCallback)
{
    OnHoverIn = InCallback;
}

void LuaButtonUI::SetOnHoverOut(std::function<void()> InCallback)
{
    OnHoverOut = InCallback;
}

void LuaButtonUI::DrawImGuiUI()
{
    if (!GetVisible())
        return;

    RectTransform worldRect = GetWorldRectTransform();
    ImVec2 pos = ImVec2(worldRect.Position.X, worldRect.Position.Y);
    ImVec2 size = ImVec2(worldRect.Size.X, worldRect.Size.Y);
    ImVec2 finalSize = bCurHoverStateIn ? ImVec2(size.x * 1.1f, size.y * 1.1f) : size;

    ImGui::SetCursorScreenPos(pos);
    bool isClicked = false;

    if (TextureID)
    {
        ImGui::PushID(this);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));        // 배경 없음
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0)); // 호버 없음
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));  // 클릭 없음

        isClicked = ImGui::ImageButton("Default", TextureID, finalSize);

        ImGui::PopStyleColor(3);
        ImGui::PopID();
    }
    else
    {
        isClicked = ImGui::Button(*GetName().ToString(), finalSize);
    }

    bool isHovered = ImGui::IsItemHovered();

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
        bCurHoverStateIn = true;
        HoverInEvent();
    }

    if (bCurHoverStateIn && !isHovered)
    {
        bCurHoverStateIn = false;
        HoverOutEvent();
    }
}


void LuaButtonUI::ButtonDownEvent()
{
    UE_LOG(ELogLevel::Display, "ButtonDownEvent");
}

void LuaButtonUI::ButtonEvent()
{
    if (OnClick)
    {
        OnClick();
    }
    else if (!LuaFunctionName.IsEmpty())
    {
        auto& Lua = FLuaScriptManager::Get().GetLua();
        if (LuaFunctionName.Len() > 0 && Lua[LuaFunctionName].valid())
        {
            auto Result = Lua[LuaFunctionName]();
            if (!Result.valid())
            {
                sol::error err = Result;
                UE_LOG(ELogLevel::Error, TEXT("Lua Error: %s"), *FString(err.what()));
            }
        }
    }
}

void LuaButtonUI::ButtonUpEvent()
{
    UE_LOG(ELogLevel::Display, "ButtonUpEvent");
}

void LuaButtonUI::HoverInEvent()
{
    if (OnHoverIn) OnHoverIn();
    UE_LOG(ELogLevel::Display, "HoverInEvent");
}

void LuaButtonUI::HoverEvent()
{
    // 일반적으로는 여기서 매 프레임 처리할 일 없음
}

void LuaButtonUI::HoverOutEvent()
{
    if (OnHoverOut) OnHoverOut();
    UE_LOG(ELogLevel::Display, "HoverOutEvent");
}
