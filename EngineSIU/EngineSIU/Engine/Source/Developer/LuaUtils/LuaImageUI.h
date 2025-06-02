#pragma once

#include "LuaUI.h"
#include "Engine/Source/Runtime/Core/Math/Color.h"

struct FTexture;

class LuaImageUI : public LuaUI 
{
public:
    LuaImageUI(FName InName);
    LuaImageUI(FName InName, RectTransform InRectTransform, int InSortOrder, FTexture* InTexture, FLinearColor& InColor);

    void PlaySubUV();
    void StopSubUV();
    void SetSubUVAnimation(int InCols, int InRows, float InFrameTime, bool bLoop);
    void SetSubUVFromFrame(int FrameIndex);

    virtual void DrawImGuiUI() override;

public:
    FTexture* Texture;
    FLinearColor Color;

public:
    void SetTexture(FTexture* InTexture);
    void SetColor(FLinearColor& InColor);
    void SetTextureByName(FString TextureName);


    int SubUVRows = 1;
    int SubUVCols = 1;
    int CurrentFrame = 0;
    float SubUVFrameTime = 0.1f; // 프레임 간 시간
    float SubUVElapsedTime = 0.f;
    bool bSubUVLoop = true;
    bool bSubUVPlaying = false;

    FVector2D UVMin = FVector2D(0.f, 0.f);
    FVector2D UVMax = FVector2D(1.f, 1.f);
};
