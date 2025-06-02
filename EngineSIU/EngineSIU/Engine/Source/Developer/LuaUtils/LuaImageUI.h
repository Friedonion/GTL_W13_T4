#pragma once

#include "LuaUI.h"
#include "Engine/Source/Runtime/Core/Math/Color.h"

struct FTexture;

class LuaImageUI : public LuaUI 
{
public:
    LuaImageUI(FName InName);
    LuaImageUI(FName InName, RectTransform InRectTransform, int InSortOrder, FTexture* InTexture, FLinearColor& InColor);

    virtual void DrawImGuiUI() override;

public:
    FTexture* Texture;
    FLinearColor Color;

public:
    void SetTexture(FTexture* InTexture);
    void SetColor(FLinearColor& InColor);
    void SetTextureByName(FString TextureName);
};
