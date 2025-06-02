#include "LuaUI.h"
#include "LuaScripts/LuaUIManager.h"

void LuaUI::Create()
{
    Visible = true;
}

FName LuaUI::GetName()
{
    return Name;
}

FString LuaUI::GetNameStr()
{
    return Name.ToString();
}

void LuaUI::SetPosition(float X, float Y)
{
    Rect.Position.X = X;
    Rect.Position.Y = Y;
}

void LuaUI::SetSize(float Width, float Height)
{
    Rect.Size.X = Width;
    Rect.Size.Y = Height;
}

void LuaUI::SetAnchorDir(AnchorDirection InAnchorDir)
{
    Rect.AnchorDir = InAnchorDir;
}

void LuaUI::SetSortOrder(int InSortOrder)
{
    SortOrder = InSortOrder;
}

void LuaUI::DrawImGuiUI()
{
}




RectTransform LuaUI::GetWorldRectTransform()
{
    RectTransform ParentRect;
    if (ParentUI == nullptr) 
    {
        ParentRect = LuaUIManager::Get().GetCanvasRectTransform();
    }
    else
    {
        ParentRect = ParentUI->GetWorldRectTransform();
    }

    // ImGui에 대응 및 계산 간략화 목적으로 
    // GlobalRect은 필수적으로 왼쪽 상단을 (0, 0), 오른쪽 하단을 (Width, Height)으로 보는 좌표계를 사용
    RectTransform GlobalRect;
    GlobalRect.AnchorDir = TopLeft;
    GlobalRect.Size = Rect.Size;    // 크기는 로컬 크기 그대로 유지

    switch (Rect.AnchorDir) 
    {
    case TopLeft:
        GlobalRect.Position.X = ParentRect.Position.X + Rect.Position.X;
        GlobalRect.Position.Y = ParentRect.Position.Y + Rect.Position.Y;
        break;
    case TopCenter:
        GlobalRect.Position.X = ParentRect.Position.X + ParentRect.Size.X * 0.5f + Rect.Position.X;
        GlobalRect.Position.Y = ParentRect.Position.Y + Rect.Position.Y;
        break;
    case TopRight:
        GlobalRect.Position.X = ParentRect.Position.X + ParentRect.Size.X + Rect.Position.X;
        GlobalRect.Position.Y = ParentRect.Position.Y + Rect.Position.Y;
        break;
    case MiddleLeft:
        GlobalRect.Position.X = ParentRect.Position.X + Rect.Position.X;
        GlobalRect.Position.Y = ParentRect.Position.Y + ParentRect.Size.Y * 0.5f + Rect.Position.Y;
        break;
    case MiddleCenter:
        GlobalRect.Position.X = ParentRect.Position.X + ParentRect.Size.X * 0.5f + Rect.Position.X;
        GlobalRect.Position.Y = ParentRect.Position.Y + ParentRect.Size.Y * 0.5f + Rect.Position.Y;
        break;
    case MiddleRight:
        GlobalRect.Position.X = ParentRect.Position.X + ParentRect.Size.X + Rect.Position.X;
        GlobalRect.Position.Y = ParentRect.Position.Y + ParentRect.Size.Y * 0.5f + Rect.Position.Y;
        break;
    case BottomLeft:
        GlobalRect.Position.X = ParentRect.Position.X + Rect.Position.X;
        GlobalRect.Position.Y = ParentRect.Position.Y + ParentRect.Size.Y + Rect.Position.Y;
        break;
    case BottomCenter:
        GlobalRect.Position.X = ParentRect.Position.X + ParentRect.Size.X * 0.5f + Rect.Position.X;
        GlobalRect.Position.Y = ParentRect.Position.Y + ParentRect.Size.Y + Rect.Position.Y;
        break;
    case BottomRight:
        GlobalRect.Position.X = ParentRect.Position.X + ParentRect.Size.X + Rect.Position.X;
        GlobalRect.Position.Y = ParentRect.Position.Y + ParentRect.Size.Y + Rect.Position.Y;
        break;
    
    default:
        GlobalRect.Position.X = ParentRect.Position.X + Rect.Position.X;
        GlobalRect.Position.Y = ParentRect.Position.Y + Rect.Position.Y;
        break;
    }
   
    return GlobalRect;
}

void LuaUI::SetParent(LuaUI* InParent)
{
    ParentUI = InParent;
    ParentUI->AddChild(this);
}

void LuaUI::AddChild(LuaUI* InChild)
{
    ChildrenUI.Add(InChild);
}
