#include "LuaUIManager.h"
#include "Engine/Source/Runtime/Core/Math/Color.h"
#include "Engine/Engine.h"
#include "Engine/Source/Developer/LuaUtils/LuaTextUI.h"
#include "Engine/Source/Developer/LuaUtils/LuaImageUI.h"
#include "Engine/Source/Developer/LuaUtils/LuaButtonUI.h"
#include "Engine/Classes/Engine/Texture.h"
#include "Engine/EditorEngine.h"
#include "Editor/LevelEditor/SLevelEditor.h"
#include "Engine/Engine.h"

#include "Actors/GameManager.h"
#include "World/World.h"
#include "Launch/EngineLoop.h"

void LuaUIManager::CreateUI(FName InName)
{
    UpdateUIArrayForSort();
}

void LuaUIManager::CreateText(FName InName, RectTransform InRectTransform, int InSortOrder, FString InText, FName FontStyleName, float InFontSize, FLinearColor InFontColor)
{
    ImFont* FindFont = GetFontStyleByName(FontStyleName);

    LuaTextUI* NewTextUI =  new LuaTextUI(InName, InRectTransform, InText, InSortOrder, FindFont, InFontSize, InFontColor);

    UIMap.Add(InName, NewTextUI);
    UpdateUIArrayForSort();
}

void LuaUIManager::CreateImage(FName InName, RectTransform InRectTransform, int InSortOrder, FName TextureName, FLinearColor InTextureColor)
{
    FTexture* FindTexture = GetTextureByName(TextureName);
    
    LuaImageUI* NewImageUI = new LuaImageUI(InName, InRectTransform, InSortOrder, FindTexture, InTextureColor);

    UIMap.Add(InName, NewImageUI);
    UpdateUIArrayForSort();
}

void LuaUIManager::CreateButton(FName InName,  RectTransform InRectTransform, int InSortOrder, FString LuaFunctionName)
{
    LuaButtonUI* NewButtonUI = new LuaButtonUI(InName, InRectTransform, InSortOrder, LuaFunctionName);

    UIMap.Add(InName, NewButtonUI);
    UpdateUIArrayForSort();
}

void LuaUIManager::DeleteUI(FName InName)
{
    // 이미 PendingDestroyUIs에 InName이 없을 때만 추가
    if (!PendingDestroyUIs.Contains(InName))
    {
        PendingDestroyUIs.Add(InName);
    }
}

void LuaUIManager::ActualDeleteUIs()
{
    if (PendingDestroyUIs.Num() == 0) return;

    for (auto DestroyName : PendingDestroyUIs) 
    {
        LuaUI** FoundPtr = UIMap.Find(DestroyName);
        if (FoundPtr != nullptr && *FoundPtr != nullptr)
        {
            delete* FoundPtr;
        }

        UIMap.Remove(DestroyName);
    }

    PendingDestroyUIs.Empty();

    UpdateUIArrayForSort();
}

LuaTextUI* LuaUIManager::GetTextUI(FName FindName)
{
    LuaUI** FoundPtr = UIMap.Find(FindName);
    if (FoundPtr == nullptr || *FoundPtr == nullptr)
    {
        return nullptr;
    }

    return static_cast<LuaTextUI*>(*FoundPtr);
}

LuaImageUI* LuaUIManager::GetImageUI(FName FindName)
{
    LuaUI** FoundPtr = UIMap.Find(FindName);
    if (FoundPtr == nullptr || *FoundPtr == nullptr)
    {
        return nullptr;
    }

    return static_cast<LuaImageUI*>(*FoundPtr);
}

LuaButtonUI* LuaUIManager::GetButtonUI(FName FindName)
{
    LuaUI** FoundPtr = UIMap.Find(FindName);
    if (FoundPtr == nullptr || *FoundPtr == nullptr)
    {
        return nullptr;
    }

    return static_cast<LuaButtonUI*>(*FoundPtr);
}

void LuaUIManager::ClearLuaUI()
{
    UIMap.Empty();
    UIArrayForSort.Empty();
}

void LuaUIManager::DrawLuaUIs()
{
    ImGuiIO& io = ImGui::GetIO();

    CanvasRectTransform.Size.X = io.DisplaySize.x; // 게임 화면의 너비(X)를 가져와 CanvasRectTransform에 설정
    CanvasRectTransform.Size.Y = io.DisplaySize.y; // 게임 화면의 높이(Y)를 가져와 CanvasRectTransform에 설정

    //// AGameManager 스폰 로직을 추가
    //static bool bGameManagerSpawnedOnce = false;

    //// 현재 활성화된 게임 월드의 포인터를 가져옵니다.
    //// GEngine은 엔진의 전역 포인터이며, ActiveWorld를 통해 현재 월드를 가져옵니다.
    //UWorld* CurrentWorld = GEngine->ActiveWorld;
    //if (GEngine != nullptr) // GEngine 전역 객체가 유효한지 먼저 확인
    //{
    //    CurrentWorld = GEngine->ActiveWorld; // GEngine이 유효하다면 ActiveWorld를 시도
    //    // 또는, 여러분의 엔진에 UWorld* GetWorld() 함수가 전역적으로 제공된다면 그것을 사용합니다.
    //    // 예를 들어, Unrean Engine은 GWorld 같은 전역 포인터가 있습니다.
    //}

    //// 만약 월드가 유효하고, AGameManager가 아직 스폰되지 않았다면
    //if (CurrentWorld != nullptr && !bGameManagerSpawnedOnce)
    //{
    //    AGameManager* SpawnedManager = CurrentWorld->SpawnActor<AGameManager>();

    //    if (SpawnedManager) // 액터 스폰에 성공했다면
    //    {
    //        SpawnedManager->SetActorLabel(TEXT("AGameManager_AutoSpawned"));
    //        bGameManagerSpawnedOnce = true;
    //    }
    //}

    ImGuiWindowFlags WindowFlags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBackground | 
        ImGuiWindowFlags_NoInputs;  // 일단 입력 가로채는 문제 있어서 추가함 이후에 수정 필요할지도

    ImGui::SetNextWindowPos(ImVec2(-5, -5), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x + 10, io.DisplaySize.y + 10), ImGuiCond_Always);

    ImGui::Begin("LuaUI", nullptr, WindowFlags);

    for (LuaUI* UI : UIArrayForSort)
    {
        if (UI != nullptr && UI->GetVisible()) 
        {
            UI->DrawImGuiUI();
        }
    }

    ImGui::End();
}

void LuaUIManager::TestCODE()
{
   // CreateText("TestTEXT", RectTransform(0, 0, 100, 100, AnchorDirection::MiddleCenter), 10, FString("+"), FName("Default"), 100, FLinearColor(1, 0, 0, 1));
    //CreateImage("TestImage", RectTransform(0, 0, 200, 200, AnchorDirection::MiddleCenter), 3, FName("ExplosionColor"), FLinearColor(1, 1, 1, 1));
    //CreateButton("TestButton", RectTransform(-100, -100, 200, 200, AnchorDirection::MiddleCenter), 15, FString("TEstbutonFUn"));
    CreateImage("TestImage2", RectTransform(0, 0, 50, 50, AnchorDirection::MiddleCenter), 3, FName("Aim"), FLinearColor(1, 0, 0, 1));

    auto GotsText = GetTextUI("TestTEXT");
    auto GotsImage = GetImageUI("TestImage");
    auto GotsButton = GetButtonUI("TestButton");

    if (GotsImage)
    {
        // 4x4 SubUV 애니메이션 설정 (16프레임, 0.05초 간격, 루프)
        GotsImage->SetSubUVAnimation(6, 6, 0.05f, true);
        GotsImage->PlaySubUV();
    }

    /*DeleteUI("TestTEXT");
    DeleteUI("TestImage");
    DeleteUI("TestButton");*/

    //auto GotText = GetTextUI("TestTEXT");
    //auto GotImage = GetImageUI("TestImage");
    //auto GotButton = GetButtonUI("TestButton");

}

void LuaUIManager::UpdateCanvasRectTransform(HWND hWnd)
{
    // 창의 크기만큼 크기 지정
    // Direction은 왼쪽 상단으로 지정하여 바로 코드에 적용할 수 있도록 함

    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    uint32 width = 0;
    uint32 height = 0;
    GEngineLoop.GetLevelEditor()->GetViewportSize(width, height);

    CanvasRectTransform.Size.X = width;
    CanvasRectTransform.Size.Y = height;
}

ImFont* LuaUIManager::GetFontStyleByName(FName FontName)
{
    ImFont** FoundPtr = FontMap.Find(FontName);

    if (FoundPtr != nullptr)
    {
        return *FoundPtr;  
    }
    else
    {
        return nullptr; 
    }
}

FTexture* LuaUIManager::GetTextureByName(FName TextureName)
{
    auto SharedPtrToTexture = TextureMap.Find(TextureName);
    if (SharedPtrToTexture == nullptr)
    {
        return nullptr;
    }

    FTexture* RawPtr = SharedPtrToTexture->get();
    if (RawPtr != nullptr)
    {
        return RawPtr;
    }

    return nullptr;
}

void LuaUIManager::InitializeResourcesAndGameManager()
{
    /* Pre Setup */
    ImGuiIO& io = ImGui::GetIO(); // ImGuiIO는 GenerateResource에서 필요하므로 이곳에 둡니다.

    /** Font load */
    ImFont* AddFont = io.Fonts->Fonts[0];
    FontMap.Add(FName("Default"), AddFont);

    /* Texture Setup*/
    TextureMap.Add(FName("ExplosionColor"), FEngineLoop::ResourceManager.GetTexture(L"Assets/Texture/T_Explosion_SubUV.png"));
    TextureMap.Add(FName("Aim"), FEngineLoop::ResourceManager.GetTexture(L"Assets/Texture/Aim.png"));
    TextureMap.Add(FName("HP_SubUV_Effect"), FEngineLoop::ResourceManager.GetTexture(L"Assets/Texture/VFX/T_FX_Heal_SubUV.png"));
    // ----------------------------------------------------

    static bool bGameManagerSpawnedOnce = false;

    // 현재 활성화된 게임 월드의 포인터를 가져옵니다.
    UWorld* CurrentWorld = GEngine->ActiveWorld;

    if (CurrentWorld != nullptr && !bGameManagerSpawnedOnce)
    {
        AGameManager* SpawnedManager = CurrentWorld->SpawnActor<AGameManager>();

        if (SpawnedManager)
        {
            SpawnedManager->SetActorLabel(TEXT("AGameManager_AutoSpawned"));
            bGameManagerSpawnedOnce = true;
        }
    }
}

LuaUIManager::LuaUIManager()
{
    CanvasRectTransform.AnchorDir = TopLeft;
    CanvasRectTransform.Position.X = 0.f;
    CanvasRectTransform.Position.Y = 0.f;

    //uint32 width = 0;
    //uint32 height = 0;
    //GEngineLoop.GetLevelEditor()->GetViewportSize(width, height);

    //CanvasRectTransform.Size.X = width;
    //CanvasRectTransform.Size.Y = height;

    CanvasRectTransform.Size.X = 0;
    CanvasRectTransform.Size.Y = 0;

    //GenerateResource();

    //TestCODE();
}

void LuaUIManager::GenerateResource()
{
}

void LuaUIManager::UpdateUIArrayForSort()
{
    UIArrayForSort.Empty();

    for (auto& Pair : UIMap) 
    {
        UIArrayForSort.Add(Pair.Value);
    }

    UIArrayForSort.Sort(
        [](LuaUI* A, LuaUI* B) -> bool
        {
            // SortOrder 값이 낮은 순서대로 그려야 하므로,
            // A가 B보다 낮으면 true를 반환
            return A->GetSortOrder() < B->GetSortOrder();
        }
    );
}
