#pragma once
#include "HAL/PlatformType.h"
#include "Math/Vector.h"


enum class ECursorType : uint8
{
    None = 0,
    Arrow,           // IDC_ARROW
    TextEditBeam,    // IDC_IBEAM
    Crosshair,      // IDC_CROSS
    Hand,            // IDC_HAND
    ResizeLeftRight, // IDC_SIZEWE
    ResizeUpDown,    // IDC_SIZENS
    ResizeSouthEast, // IDC_SIZENWSE
    ResizeSouthWest, // IDC_SIZENESW
    ResizeAll,       // IDC_SIZEALL
    Max
};

/**
 * WinAPI에서 Cursor관련 로직을 래핑한 클래스 입니다. 
 */
struct FWindowsCursor
{
    /** 현재 Cursor의 위치를 가져옵니다. */
    static FVector2D GetPosition();

    /** Cursor의 위치를 설정합니다. */
    static void SetPosition(const int32 X, const int32 Y);

    /** 현재 클라이언트 기준으로 Cursor의 위치를 가져옵니다. */
    static FVector2D GetClientPosition();

    /** 현재 클라이언트의 기준으로 Cursor의 위치를 설정합니다. */
    static void SetClientPosition(const int32 X, const int32 Y);

    /** Cursor의 모양을 가져옵니다. */
    static ECursorType GetMouseCursor();

    /** Cursor의 모양을 설정합니다. */
    static void SetMouseCursor(const ECursorType CursorType = ECursorType::Arrow);

    /** Cursor의 표시 여부를 반환합니다. */
    static bool GetShowMouseCursor();

    /** Cursor의 표시 여부를 설정합니다. */
    static void SetShowMouseCursor(bool ShowCursor);

private:
    /** Cursor의 표시 여부 */
    static bool bShowCursor;
};
