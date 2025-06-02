#include "WindowsCursor.h"

#include "Define.h"
#include "UObject/Object.h"

bool FWindowsCursor::bShowCursor = true;


FVector2D FWindowsCursor::GetPosition()
{
    POINT CursorPos;
    ::GetCursorPos(&CursorPos);

    return {
        static_cast<float>(CursorPos.x),
        static_cast<float>(CursorPos.y)
    };
}

void FWindowsCursor::SetPosition(const int32 X, const int32 Y)
{
    ::SetCursorPos(X, Y);
}

FVector2D FWindowsCursor::GetClientPosition()
{
    POINT CursorPos;
    ::GetCursorPos(&CursorPos);
    ::ScreenToClient(GEngineLoop.AppWnd, &CursorPos);

    return {
        static_cast<float>(CursorPos.x),
        static_cast<float>(CursorPos.y)
    };
}

void FWindowsCursor::SetClientPosition(const int32 X, const int32 Y)
{
    POINT CursorPos = { X, Y };
    ::ClientToScreen(GEngineLoop.AppWnd, &CursorPos);
    ::SetCursorPos(CursorPos.x, CursorPos.y);
}

ECursorType FWindowsCursor::GetMouseCursor()
{
    const HCURSOR Cursor = ::GetCursor();
    if (Cursor == nullptr)
    {
        return ECursorType::None;
    }

    // 시스템 커서 핸들과 비교
    if (Cursor == LoadCursor(nullptr, IDC_ARROW))
    {
        return ECursorType::Arrow;
    }
    else if (Cursor == LoadCursor(nullptr, IDC_IBEAM))
    {
        return ECursorType::TextEditBeam;
    }
    else if (Cursor == LoadCursor(nullptr, IDC_CROSS))
    {
        return ECursorType::Crosshair;
    }
    else if (Cursor == LoadCursor(nullptr, IDC_HAND))
    {
        return ECursorType::Hand;
    }
    else if (Cursor == LoadCursor(nullptr, IDC_SIZEWE))
    {
        return ECursorType::ResizeLeftRight;
    }
    else if (Cursor == LoadCursor(nullptr, IDC_SIZENS))
    {
        return ECursorType::ResizeUpDown;
    }
    else if (Cursor == LoadCursor(nullptr, IDC_SIZENWSE))
    {
        return ECursorType::ResizeSouthEast; // 실제 WinAPI는 NW-SE 방향
    }
    else if (Cursor == LoadCursor(nullptr, IDC_SIZENESW))
    {
        return ECursorType::ResizeSouthWest; // 실제 WinAPI는 NE-SW 방향
    }
    else if (Cursor == LoadCursor(nullptr, IDC_SIZEALL))
    {
        return ECursorType::ResizeAll;
    }

    return ECursorType::None;
}


void FWindowsCursor::SetMouseCursor(const ECursorType CursorType)
{
    HCURSOR NewCursor = nullptr;

    // 커서 타입별 핸들 매핑
    switch (CursorType)
    {
    case ECursorType::Arrow:
        NewCursor = LoadCursor(nullptr, IDC_ARROW);
        break;
    case ECursorType::TextEditBeam:
        NewCursor = LoadCursor(nullptr, IDC_IBEAM);
        break;
    case ECursorType::Crosshair:
        NewCursor = LoadCursor(nullptr, IDC_CROSS);
        break;
    case ECursorType::Hand:
        NewCursor = LoadCursor(nullptr, IDC_HAND);
        break;
    case ECursorType::ResizeLeftRight:
        NewCursor = LoadCursor(nullptr, IDC_SIZEWE);
        break;
    case ECursorType::ResizeUpDown:
        NewCursor = LoadCursor(nullptr, IDC_SIZENS);
        break;
    case ECursorType::ResizeSouthEast:
        NewCursor = LoadCursor(nullptr, IDC_SIZENWSE); // 실제 WinAPI는 NW-SE 방향
        break;
    case ECursorType::ResizeSouthWest:
        NewCursor = LoadCursor(nullptr, IDC_SIZENESW); // 실제 WinAPI는 NE-SW 방향
        break;
    case ECursorType::ResizeAll:
        NewCursor = LoadCursor(nullptr, IDC_SIZEALL);
        break;
    case ECursorType::None:
    case ECursorType::Max:
        NewCursor = nullptr; // 커서 숨김
        break;
    }

    ::SetCursor(NewCursor);
}

bool FWindowsCursor::GetShowMouseCursor()
{
    return bShowCursor;
}

void FWindowsCursor::SetShowMouseCursor(bool ShowCursor)
{
    if (bShowCursor != ShowCursor)
    {
        ::ShowCursor(ShowCursor);
        bShowCursor = ShowCursor;
    }
}
