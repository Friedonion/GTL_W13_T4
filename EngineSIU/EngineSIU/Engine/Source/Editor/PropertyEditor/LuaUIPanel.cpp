#include "LuaUIPanel.h"
#include "LuaScripts/LuaUIManager.h"


LuaUIViewPanel::LuaUIViewPanel()
{
    SetSupportedWorldTypes(EWorldTypeBitFlag::PIE );
}

void LuaUIViewPanel::Render()
{
    LuaUIManager::Get().ActualDeleteUIs();
    LuaUIManager::Get().DrawLuaUIs();
}

void LuaUIViewPanel::OnResize(HWND hWnd)
{
    LuaUIManager::Get().UpdateCanvasRectTransform(hWnd);
}
