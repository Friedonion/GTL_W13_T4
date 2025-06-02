#pragma once

#include "Engine/Source/Developer/LuaUtils/LuaUI.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/NameTypes.h"
#include "Engine/Source/Runtime/Core/Container/Map.h"
#include "Engine/Source/ThirdParty/ImGui/include/ImGui/imgui.h"

struct FTexture;

class LuaTextUI;
class LuaImageUI;
class LuaButtonUI;

class LuaUIManager 
{
public:
    static LuaUIManager& Get()
    {
        static LuaUIManager Instance;
        return Instance;
    }


    void CreateUI(FName InName);
    // Text의 경우 크기는 FontSize에만 따라가도록 일단 구현
    void CreateText(FName InName, RectTransform InRectTransform, int InSortOrder, FString InText, FName FontStyleName, float InFontSize, FLinearColor InFontColor);
    void CreateImage(FName InName, RectTransform InRectTransform, int InSortOrder, FName TextureName, FLinearColor InTextureColor);
    void CreateButton(FName InName, RectTransform InRectTransform, int InSortOrder, FString LuaFunctionName);

    void DeleteUI(FName InName);

    void ActualDeleteUIs();
    
    LuaTextUI* GetTextUI(FName FindName);
    LuaImageUI* GetImageUI(FName FindName);
    LuaButtonUI* GetButtonUI(FName FindName);
    
    void ClearLuaUI();

    void DrawLuaUIs();

    /////////////// TEST CODE 꼭 지우기
    void TestCODE();

    void UpdateCanvasRectTransform(HWND hWnd);

    RectTransform GetCanvasRectTransform() { return CanvasRectTransform; }

    ImFont* GetFontStyleByName(FName FontName);
    FTexture* GetTextureByName(FName TextureName);

private:
    LuaUIManager();
    ~LuaUIManager() = default;
    LuaUIManager(const LuaUIManager&) = delete;
    LuaUIManager& operator=(const LuaUIManager&) = delete;

    void GenerateResource();
    void UpdateUIArrayForSort();    // Create 하거나 SortOrder 바꿀 때에 호출 필요

    TMap<FName, LuaUI*> UIMap;
    TArray<LuaUI*> UIArrayForSort;  // Sort하여 Render하기 위해 Array 사용

    RectTransform CanvasRectTransform;

    TMap<FName, ImFont*> FontMap;
    TMap<FName, std::shared_ptr<FTexture>> TextureMap;

    TArray<FName> PendingDestroyUIs;
};
