#pragma once
#include "Container/Map.h"
#include "HAL/PlatformType.h"


namespace EMouseButtons
{
enum Type : uint8
{
    Left = 0,
    Middle,
    Right,
    Thumb01,
    Thumb02,

    Invalid,
};
}

enum EInputEvent : uint8
{
    IE_Pressed = 0,
    IE_Released = 1,
    IE_Repeat = 2,
    IE_DoubleClick = 3,
    IE_Axis = 4,
    IE_None = 5,
    IE_MAX = 6,
};

/**
 * ModifierKeys를 구성하는 키의 상태를 나타내는 구조체입니다.
 */
struct FModifierKeysState
{
public:
    FModifierKeysState(
        const bool bInIsLeftShiftDown,
        const bool bInIsRightShiftDown,
        const bool bInIsLeftControlDown,
        const bool bInIsRightControlDown,
        const bool bInIsLeftAltDown,
        const bool bInIsRightAltDown,
        const bool bInIsLeftWinDown,
        const bool bInIsRightWinDown,
        const bool bInAreCapsLocked
    )
        : bIsLeftShiftDown(static_cast<uint16>(bInIsLeftShiftDown))
        , bIsRightShiftDown(static_cast<uint16>(bInIsRightShiftDown))
        , bIsLeftControlDown(static_cast<uint16>(bInIsLeftControlDown))
        , bIsRightControlDown(static_cast<uint16>(bInIsRightControlDown))
        , bIsLeftAltDown(static_cast<uint16>(bInIsLeftAltDown))
        , bIsRightAltDown(static_cast<uint16>(bInIsRightAltDown))
        , bIsLeftWinDown(static_cast<uint16>(bInIsLeftWinDown))
        , bIsRightWinDown(static_cast<uint16>(bInIsRightWinDown))
        , bAreCapsLocked(static_cast<uint16>(bInAreCapsLocked))
    {
    }

    FModifierKeysState()
        : bIsLeftShiftDown(false)
        , bIsRightShiftDown(false)
        , bIsLeftControlDown(false)
        , bIsRightControlDown(false)
        , bIsLeftAltDown(false)
        , bIsRightAltDown(false)
        , bIsLeftWinDown(false)
        , bIsRightWinDown(false)
        , bAreCapsLocked(false)
    {
    }

    /**
     * Shift 키 중 하나라도 눌려 있었는지 여부를 반환합니다.
     *
     * @return  Shift 키가 눌려 있으면 true
     */
    bool IsShiftDown() const
    {
        return bIsLeftShiftDown || bIsRightShiftDown;
    }

    /**
     * 왼쪽 Shift 키가 눌려 있었는지 여부를 반환합니다.
     *
     * @return  왼쪽 Shift 키가 눌려 있으면 true
     */
    bool IsLeftShiftDown() const
    {
        return bIsLeftShiftDown;
    }

    /**
     * 오른쪽 Shift 키가 눌려 있었는지 여부를 반환합니다.
     *
     * @return  오른쪽 Shift 키가 눌려 있으면 true
     */
    bool IsRightShiftDown() const
    {
        return bIsRightShiftDown;
    }

    /**
     * Control 키 중 하나라도 눌려 있었는지 여부를 반환합니다.
     *
     * @return  Control 키가 눌려 있으면 true
     */
    bool IsControlDown() const
    {
        return bIsLeftControlDown || bIsRightControlDown;
    }

    /**
     * 왼쪽 Control 키가 눌려 있었는지 여부를 반환합니다.
     *
     * @return  왼쪽 Control 키가 눌려 있으면 true
     */
    bool IsLeftControlDown() const
    {
        return bIsLeftControlDown;
    }

    /**
     * 오른쪽 Control 키가 눌려 있었는지 여부를 반환합니다.
     *
     * @return  오른쪽 Control 키가 눌려 있으면 true
     */
    bool IsRightControlDown() const
    {
        return bIsRightControlDown;
    }

    /**
     * Alt 키 중 하나라도 눌려 있었는지 여부를 반환합니다.
     *
     * @return  Alt 키가 눌려 있으면 true
     */
    bool IsAltDown() const
    {
        return bIsLeftAltDown || bIsRightAltDown;
    }

    /**
     * 왼쪽 Alt 키가 눌려 있었는지 여부를 반환합니다.
     *
     * @return  왼쪽 Alt 키가 눌려 있으면 true
     */
    bool IsLeftAltDown() const
    {
        return bIsLeftAltDown;
    }

    /**
     * 오른쪽 Alt 키가 눌려 있었는지 여부를 반환합니다.
     *
     * @return  오른쪽 Alt 키가 눌려 있으면 true
     */
    bool IsRightAltDown() const
    {
        return bIsRightAltDown;
    }

    /**
     * Windows 키 중 하나라도 눌려 있었는지 여부를 반환합니다.
     *
     * @return  Windows 키가 눌려 있으면 true
     */
    bool IsWindowsKeyDown() const
    {
        return bIsLeftWinDown || bIsRightWinDown;
    }

    /**
     * 왼쪽 Windows 키가 눌려 있었는지 여부를 반환합니다.
     *
     * @return  왼쪽 Windows 키가 눌려 있으면 true
     */
    bool IsLeftWinDown() const
    {
        return bIsLeftWinDown;
    }

    /**
     * 오른쪽 Windows 키가 눌려 있었는지 여부를 반환합니다.
     *
     * @return  오른쪽 Windows 키가 눌려 있으면 true
     */
    bool IsRightWinDown() const
    {
        return bIsRightWinDown;
    }

    /**
     * Caps Lock 키가 활성화 상태인지 여부를 반환합니다.
     *
     * @return  Caps Lock이 활성화되어 있으면 true
     */
    bool AreCapsLocked() const
    {
        return bAreCapsLocked;
    }

private:
    uint16 bIsLeftShiftDown : 1;
    uint16 bIsRightShiftDown : 1;
    uint16 bIsLeftControlDown : 1;
    uint16 bIsRightControlDown : 1;
    uint16 bIsLeftAltDown : 1;
    uint16 bIsRightAltDown : 1;
    uint16 bIsLeftWinDown : 1;
    uint16 bIsRightWinDown : 1;
    uint16 bAreCapsLocked : 1;
};

namespace EKeys
{
enum Type : uint8
{
    Invalid = 0, // 유효하지 않은 키
    AnyKey,      // 모든 키

    MouseX,          // 마우스 X축 이동
    MouseY,          // 마우스 Y축 이동
    Mouse2D,         // 마우스 2D 축
    MouseScrollUp,   // 마우스 휠 업
    MouseScrollDown, // 마우스 휠 다운
    MouseWheelAxis,  // 마우스 휠 축

    LeftMouseButton,   // 왼쪽 마우스 버튼
    RightMouseButton,  // 오른쪽 마우스 버튼
    MiddleMouseButton, // 중간 마우스 버튼
    ThumbMouseButton,  // 측면 마우스 버튼 1
    ThumbMouseButton2, // 측면 마우스 버튼 2

    BackSpace, // 백스페이스
    Tab,       // 탭
    Enter,     // 엔터
    Pause,     // 일시정지

    CapsLock, // 캡스락
    Escape,   // ESC
    SpaceBar, // 스페이스바
    PageUp,   // 페이지 업
    PageDown, // 페이지 다운
    End,      // End 키
    Home,     // Home 키

    Left,  // 왼쪽 화살표
    Up,    // 위쪽 화살표
    Right, // 오른쪽 화살표
    Down,  // 아래쪽 화살표

    Insert, // Insert 키
    Delete, // Delete 키

    Zero,  // 0
    One,   // 1
    Two,   // 2
    Three, // 3
    Four,  // 4
    Five,  // 5
    Six,   // 6
    Seven, // 7
    Eight, // 8
    Nine,  // 9

    A, // A 키
    B, // B 키
    C, // C 키
    D, // D 키
    E, // E 키
    F, // F 키
    G, // G 키
    H, // H 키
    I, // I 키
    J, // J 키
    K, // K 키
    L, // L 키
    M, // M 키
    N, // N 키
    O, // O 키
    P, // P 키
    Q, // Q 키
    R, // R 키
    S, // S 키
    T, // T 키
    U, // U 키
    V, // V 키
    W, // W 키
    X, // X 키
    Y, // Y 키
    Z, // Z 키

    NumPadZero,  // 넘패드 0
    NumPadOne,   // 넘패드 1
    NumPadTwo,   // 넘패드 2
    NumPadThree, // 넘패드 3
    NumPadFour,  // 넘패드 4
    NumPadFive,  // 넘패드 5
    NumPadSix,   // 넘패드 6
    NumPadSeven, // 넘패드 7
    NumPadEight, // 넘패드 8
    NumPadNine,  // 넘패드 9

    Multiply, // 곱셈(*) 키
    Add,      // 덧셈(+) 키
    Subtract, // 뺄셈(-) 키
    Decimal,  // 소수점(.) 키
    Divide,   // 나눗셈(/) 키

    F1,  // F1 키
    F2,  // F2 키
    F3,  // F3 키
    F4,  // F4 키
    F5,  // F5 키
    F6,  // F6 키
    F7,  // F7 키
    F8,  // F8 키
    F9,  // F9 키
    F10, // F10 키
    F11, // F11 키
    F12, // F12 키

    NumLock,    // 넘락 키
    ScrollLock, // 스크롤락 키

    LeftShift,    // 왼쪽 Shift
    RightShift,   // 오른쪽 Shift
    LeftControl,  // 왼쪽 Ctrl
    RightControl, // 오른쪽 Ctrl
    LeftAlt,      // 왼쪽 Alt
    RightAlt,     // 오른쪽 Alt
    LeftCommand,  // 왼쪽 Command(⌘)
    RightCommand, // 오른쪽 Command(⌘)

    Semicolon,    // 세미콜론(;)
    Equals,       // 등호(=)
    Comma,        // 쉼표(,)
    Underscore,   // 언더스코어(_)
    Hyphen,       // 하이픈(-)
    Period,       // 마침표(.)
    Slash,        // 슬래시(/)
    Tilde,        // 틸드(~)
    LeftBracket,  // 왼쪽 대괄호([)
    Backslash,    // 백슬래시(\)
    RightBracket, // 오른쪽 대괄호(])
    Apostrophe,   // 아포스트로피(')

    Ampersand,        // 앰퍼샌드(&)
    Asterix,          // 별표(*)
    Caret,            // 캐럿(^)
    Colon,            // 콜론(:)
    Dollar,           // 달러($)
    Exclamation,      // 느낌표(!)
    LeftParentheses,  // 왼쪽 괄호
    RightParentheses, // 오른쪽 괄호
    Quote,            // 더블쿼터(")

    // Gamepad Keys
    Gamepad_Left2D,           // 왼쪽 스틱 2D 축 (X+Y)
    Gamepad_LeftX,            // 왼쪽 스틱 X축
    Gamepad_LeftY,            // 왼쪽 스틱 Y축
    Gamepad_Right2D,          // 오른쪽 스틱 2D 축 (X+Y)
    Gamepad_RightX,           // 오른쪽 스틱 X축
    Gamepad_RightY,           // 오른쪽 스틱 Y축
    Gamepad_LeftTriggerAxis,  // 왼쪽 트리거 축 (아날로그 입력)
    Gamepad_RightTriggerAxis, // 오른쪽 트리거 축 (아날로그 입력)

    Gamepad_LeftThumbstick,    // 왼쪽 스틱 버튼 (클릭)
    Gamepad_RightThumbstick,   // 오른쪽 스틱 버튼 (클릭)
    Gamepad_Special_Left,      // 특수 왼쪽 버튼 (PS 홈 버튼 등)
    Gamepad_Special_Left_X,    // 특수 왼쪽 버튼 X축
    Gamepad_Special_Left_Y,    // 특수 왼쪽 버튼 Y축
    Gamepad_Special_Right,     // 특수 오른쪽 버튼 (Xbox 메뉴 버튼 등)
    Gamepad_FaceButton_Bottom, // 하단 액션 버튼 (XBOX A, PS ◯)
    Gamepad_FaceButton_Right,  // 오른쪽 액션 버튼 (XBOX B, PS ×)
    Gamepad_FaceButton_Left,   // 왼쪽 액션 버튼 (XBOX X, PS ▫)
    Gamepad_FaceButton_Top,    // 상단 액션 버튼 (XBOX Y, PS △)
    Gamepad_LeftShoulder,      // 왼쪽 숄더 버튼 (LB)
    Gamepad_RightShoulder,     // 오른쪽 숄더 버튼 (RB)
    Gamepad_LeftTrigger,       // 왼쪽 트리거 버튼 (디지털 입력)
    Gamepad_RightTrigger,      // 오른쪽 트리거 버튼 (디지털 입력)
    Gamepad_DPad_Up,           // D-Pad 위쪽
    Gamepad_DPad_Down,         // D-Pad 아래쪽
    Gamepad_DPad_Right,        // D-Pad 오른쪽
    Gamepad_DPad_Left,         // D-Pad 왼쪽

    Max,
};

// 일부 키코드만 지원
EKeys::Type FromCharCode(uint32_t CharCode);
FString ToString(EKeys::Type Key);
}

struct FInputKeyManager // ue FInputKeyManager.cpp, 1530
{
public:
    static FInputKeyManager& Get();

private:
    FInputKeyManager() = default;

    TMap<uint32, EKeys::Type> KeyMapVirtualToEnum;
    TMap<uint32, EKeys::Type> KeyMapCharToEnum;
};
