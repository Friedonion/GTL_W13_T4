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
        return;

    // SubUV 애니메이션 시간 갱신
    if (bSubUVPlaying)
    {
        SubUVElapsedTime += ImGui::GetIO().DeltaTime;
        if (SubUVElapsedTime >= SubUVFrameTime)
        {
            SubUVElapsedTime -= SubUVFrameTime;
            CurrentFrame++;
            int maxFrames = SubUVRows * SubUVCols;

            if (CurrentFrame >= maxFrames)
            {
                if (bSubUVLoop)
                    CurrentFrame = 0;
                else
                {
                    CurrentFrame = maxFrames - 1;
                    bSubUVPlaying = false;
                }
            }
            SetSubUVFromFrame(CurrentFrame);
        }
    }

    RectTransform worldRect = GetWorldRectTransform();
    ImVec2 screenPos = ImVec2(worldRect.Position.X, worldRect.Position.Y);
    ImVec2 drawSize = ImVec2(worldRect.Size.X, worldRect.Size.Y);
    ImVec4 tintColor = ImVec4(Color.R, Color.G, Color.B, Color.A);
    ImTextureID texID = (ImTextureID)(intptr_t)(Texture->TextureSRV);

    ImGui::SetCursorScreenPos(screenPos);
    ImGui::Image(texID, drawSize, ImVec2(UVMin.X, UVMin.Y), ImVec2(UVMax.X, UVMax.Y), tintColor);
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

void LuaImageUI::SetSubUVAnimation(int InCols, int InRows, float InFrameTime, bool bLoop)
{
    SubUVCols = InCols;
    SubUVRows = InRows;
    SubUVFrameTime = InFrameTime;
    bSubUVLoop = bLoop;
    CurrentFrame = 0;
    SubUVElapsedTime = 0.f;
    SetSubUVFromFrame(CurrentFrame);
}

void LuaImageUI::PlaySubUV()
{
    bSubUVPlaying = true;
}

void LuaImageUI::StopSubUV()
{
    bSubUVPlaying = false;
}

void LuaImageUI::SetSubUVFromFrame(int FrameIndex)
{
    float cellWidth = 1.0f / SubUVCols;
    float cellHeight = 1.0f / SubUVRows;

    int col = FrameIndex % SubUVCols;
    int row = FrameIndex / SubUVCols;

    UVMin = FVector2D(col * cellWidth, row * cellHeight);
    UVMax = FVector2D((col + 1) * cellWidth, (row + 1) * cellHeight);
}

