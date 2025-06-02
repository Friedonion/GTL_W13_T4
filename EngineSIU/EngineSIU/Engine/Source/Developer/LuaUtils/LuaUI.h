#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/NameTypes.h"
#include "Engine/Source/Runtime/Core/Math/Vector.h"
#include "Engine/Source/Runtime/Core/Container/Array.h"

enum AnchorDirection 
{
    TopLeft,
    TopCenter,
    TopRight,

    MiddleLeft,
    MiddleCenter,
    MiddleRight,

    BottomLeft,
    BottomCenter,
    BottomRight
};

struct RectTransform 
{
    FVector2D Position; // 로컬 위치
    FVector2D Size; // 너비 및 높이
    AnchorDirection AnchorDir;

    RectTransform()
        : Position(0, 0), Size(100, 100), AnchorDir(AnchorDirection::MiddleCenter)
    { }

    RectTransform(float PosX, float PosY, float SizeX, float SizeY, AnchorDirection InAnchorDir) 
        : Position(PosX, PosY), Size(SizeX, SizeY), AnchorDir(InAnchorDir)
    { }
};

class LuaUI 
{
public:
    LuaUI(FName InName) 
        : Name(InName), Visible(true)
    { }

protected:
    FName Name;
    bool Visible = false;
    RectTransform Rect;
    int SortOrder;  // SortOrder 값이 낮으면 먼저 렌더링됨

    LuaUI* ParentUI = nullptr;
    TArray<LuaUI*> ChildrenUI;

public:
    virtual void Create();
    FName GetName();
    FString GetNameStr();

    RectTransform& GetRectTransform() { return Rect; }

    void SetPosition(float X, float Y);
    void SetSize(float Width, float Height);
    void SetAnchorDir(AnchorDirection InAnchorDir);
    void SetSortOrder(int InSortOrder);
    
    virtual void DrawImGuiUI();

    RectTransform GetWorldRectTransform();
    int GetSortOrder() { return SortOrder; }
    bool GetVisible() { return Visible; }

    void SetParent(LuaUI* InParent);
    void AddChild(LuaUI* InChild);
};
