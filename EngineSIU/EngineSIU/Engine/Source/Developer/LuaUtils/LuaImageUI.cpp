#include "LuaImageUI.h"
#include "Engine/Classes/Engine/Texture.h"
#include "Engine/Source/ThirdParty/ImGui/include/ImGui/imgui.h"
#include "LuaScripts/LuaUIManager.h"


LuaImageUI::LuaImageUI(FName InName)
    :LuaUI(InName)
{
}

LuaImageUI::LuaImageUI(FName InName, RectTransform InRectTransform, int InSortOrder, FTexture* InTexture, FLinearColor& InColor)
    :LuaUI(InName), Texture(InTexture), Color(InColor)
{
    Rect = InRectTransform;
}

void LuaImageUI::DrawImGuiUI()
{
    if (!GetVisible() || Texture == nullptr)
    {
        return;
    }

    RectTransform worldRect = GetWorldRectTransform();
    ImVec2 screenPos = ImVec2(worldRect.Position.X, worldRect.Position.Y);
    ImVec2 drawSize = ImVec2(worldRect.Size.X, worldRect.Size.Y);

    ImVec4 tintColor = ImVec4(Color.R, Color.G, Color.B, Color.A);

    ImTextureID texID = (ImTextureID)(intptr_t)(Texture->TextureSRV);

    ImGui::SetCursorScreenPos(screenPos);
    ImGui::Image(
        texID,
        drawSize,
        ImVec2(0.0f, 0.0f),  // UV 시작
        ImVec2(1.0f, 1.0f),  // UV 끝
        tintColor           // Tint(색상 + 알파)
    );
}

void LuaImageUI::SetTexture(FTexture* InTexture)
{
    Texture = InTexture;
}

void LuaImageUI::SetColor(FLinearColor& InColor)
{
    Color = InColor;
}

void LuaImageUI::SetTextureByName(FString TextureName)
{
    FTexture* FindTexture = LuaUIManager::Get().GetTextureByName(TextureName);

    if (FindTexture != nullptr) 
    {
        Texture = FindTexture;
    }
}

