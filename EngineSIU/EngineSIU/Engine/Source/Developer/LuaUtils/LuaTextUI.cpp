#include "LuaTextUI.h"

LuaTextUI::LuaTextUI(FName InName)
    :LuaUI(InName), Text(FString("Default Text")), FontSytle(ImGui::GetFont()), FontSize(18.0f), FontColor(FLinearColor(1.0f))
{
}

LuaTextUI::LuaTextUI(FName InName, RectTransform InRectTransform, FString& InText, int InSortOrder, ImFont* InFontStyle, float InFontSize, FLinearColor InFontColor)
    :LuaUI(InName), Text(InText), FontSytle(InFontStyle), FontSize(InFontSize), FontColor(InFontColor)
{
    Visible = true;
    Rect = InRectTransform;
    SortOrder = InSortOrder;
}

void LuaTextUI::DrawImGuiUI()
{
    if (!GetVisible())
        return;

    RectTransform worldRect = GetWorldRectTransform();
    ImVec2 textPos = ImVec2(worldRect.Position.X, worldRect.Position.Y);

    ImFont* fontToUse = (FontSytle != nullptr) ? FontSytle : ImGui::GetFont();
    float sizeToUse = (FontSize > 0.0f) ? FontSize : ImGui::GetFontSize();
    
    ImU32 textColor = ImGui::GetColorU32(ImVec4(FontColor.R, FontColor.G, FontColor.B, FontColor.A));

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddText(fontToUse, sizeToUse, textPos, textColor, *Text);

}

void LuaTextUI::SetText(FString& InText)
{
    Text = InText;
}

void LuaTextUI::SetFont(ImFont* InFontStyle)
{
    FontSytle = InFontStyle;
}

void LuaTextUI::SetFontSize(float InFontSize)
{
    FontSize = InFontSize;
}

void LuaTextUI::SetFontColor(FLinearColor InFontColor)
{
    FontColor = InFontColor;
}
