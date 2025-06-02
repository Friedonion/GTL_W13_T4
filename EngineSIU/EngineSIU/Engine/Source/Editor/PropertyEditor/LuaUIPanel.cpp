#include "LuaUIPanel.h"
#include "LuaScripts/LuaUIManager.h"

#include "Actors/GameManager.h" // AGameManager 클래스의 정의
#include "World/World.h"       // UWorld 클래스와 SpawnActor
#include "Engine/Engine.h"     

LuaUIViewPanel::LuaUIViewPanel()
{
    SetSupportedWorldTypes(EWorldTypeBitFlag::PIE );
}

void LuaUIViewPanel::Render()
{
    // --- 이 초기화 함수를 Render() 시작 부분에서 호출합니다! ---
    // 이 함수는 처음 호출될 때만 내부적으로 리소스를 로드하고 GameManager를 스폰할 것입니다.
    static bool bInitializedOnce = false;
    if (!bInitializedOnce && GEngine != nullptr && GEngine->ActiveWorld != nullptr)
    {
        // GameManager를 스폰할 때 UWorld::IsGameWorld() 같은 조건으로 더 안정화할 수 있습니다.
        // 현재는 PIE에서 AGameManager를 확실히 스폰하는 것이 목표이므로 이대로 진행합니다.
        LuaUIManager::Get().InitializeResourcesAndGameManager(); // <-- 이 줄을 추가합니다!
        bInitializedOnce = true;
    }
    // -------------------------------------------------------

    LuaUIManager::Get().ActualDeleteUIs();
    // AGameManager 스폰 로직은 이제 InitializeResourcesAndGameManager() 안으로 들어갔으므로 이곳에서 제거합니다.

    LuaUIManager::Get().DrawLuaUIs();
}

void LuaUIViewPanel::OnResize(HWND hWnd)
{
    LuaUIManager::Get().UpdateCanvasRectTransform(hWnd);
}
