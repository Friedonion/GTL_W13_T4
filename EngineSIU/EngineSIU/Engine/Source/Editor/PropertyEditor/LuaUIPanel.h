#pragma once

#include "UnrealEd/EditorPanel.h"

class LuaUIViewPanel : public UEditorPanel 
{
public:
    LuaUIViewPanel();
    virtual void Render() override;
    virtual void OnResize(HWND hWnd) override;
};
