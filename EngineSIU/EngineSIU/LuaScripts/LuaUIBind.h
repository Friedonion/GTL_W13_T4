#pragma once

#include <sol/sol.hpp>

#include "LuaUIManager.h"

class LuaUIBind 
{
public:
    static void Bind(sol::table& Table);

private:
    static void CreateText(FString InName, RectTransform InRectTransform, int InSortOrder, FString InText, FString FontStyleName, float InFontSize, FLinearColor InFontColor);
    static void CreateImage(FString InName, RectTransform InRectTransform, int InSortOrder, FString TextureName, FLinearColor InTextureColor);
    static void CreateButton(FString InName, RectTransform InRectTransform, int InSortOrder, FString LuaFunctionName);

    static void DeleteUI(FString InName);

    static LuaTextUI* GetTextUI(FString FindName);
    static LuaImageUI* GetImageUI(FString FindName);
    static LuaButtonUI* GetButtonUI(FString FindName);

    static void ClearLuaUI();
};
