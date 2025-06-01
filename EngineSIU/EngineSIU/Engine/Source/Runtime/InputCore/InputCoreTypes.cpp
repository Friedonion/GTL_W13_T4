#include "InputCoreTypes.h"
#include <assert.h>

FInputKeyManager& FInputKeyManager::Get()
{
    static FInputKeyManager Instance;
    return Instance;
}

// 일부 키코드만 지원
EKeys::Type EKeys::FromCharCode(uint32_t CharCode)
{
    // 알파벳 (대소문자 구분 없이 A~Z 매핑)
    if (std::isalpha(CharCode)) {
        return static_cast<EKeys::Type>(EKeys::A + (std::toupper(CharCode) - 'A'));
    }

    // 숫자 0~9
    if (CharCode >= '0' && CharCode <= '9') {
        return static_cast<EKeys::Type>(EKeys::Zero + (CharCode - '0'));
    }

    // 특수문자 매핑
    switch (CharCode) {
    case ' ': return EKeys::SpaceBar;
    case '\t': return EKeys::Tab;
    case '\n': // '\n'도 Enter로 처리
    case '\r': return EKeys::Enter;
    case '\b': return EKeys::BackSpace;
    case 27: return EKeys::Escape; // ASCII ESC

    case ';': return EKeys::Semicolon;
    case '=': return EKeys::Equals;
    case ',': return EKeys::Comma;
    case '-': return EKeys::Hyphen;
    case '.': return EKeys::Period;
    case '/': return EKeys::Slash;
    case '~': return EKeys::Tilde;
    case '[': return EKeys::LeftBracket;
    case '\\': return EKeys::Backslash;
    case ']': return EKeys::RightBracket;
    case '\'': return EKeys::Apostrophe;

    default:
        //assert(0);
        return EKeys::Invalid;
    }
}

uint32 EKeys::ToCharCode(EKeys::Type Key)
{
    // 알파벳 (A~Z)
    if (Key >= EKeys::A && Key <= EKeys::Z) {
        return 'A' + (Key - EKeys::A);
    }

    // 숫자 0~9
    if (Key >= EKeys::Zero && Key <= EKeys::Nine) {
        return '0' + (Key - EKeys::Zero);
    }

    // 특수문자 매핑
    switch (Key) {
    case EKeys::SpaceBar:    return ' ';
    case EKeys::Tab:         return '\t';
    case EKeys::Enter:       return '\n'; // '\r'도 가능
    case EKeys::BackSpace:   return '\b';
    case EKeys::Escape:      return 27;   // ASCII ESC

    case EKeys::Semicolon:   return ';';
    case EKeys::Equals:      return '=';
    case EKeys::Comma:       return ',';
    case EKeys::Hyphen:      return '-';
    case EKeys::Period:      return '.';
    case EKeys::Slash:       return '/';
    case EKeys::Tilde:       return '~';
    case EKeys::LeftBracket: return '[';
    case EKeys::Backslash:   return '\\';
    case EKeys::RightBracket:return ']';
    case EKeys::Apostrophe:  return '\'';

    default:
        return 0; // 매핑이 없는 경우
    }
}

FString EKeys::ToString(EKeys::Type Key)
{
    switch (Key)
    {
        // 알파벳
    case EKeys::A: return TEXT("A");
    case EKeys::B: return TEXT("B");
    case EKeys::C: return TEXT("C");
    case EKeys::D: return TEXT("D");
    case EKeys::E: return TEXT("E");
    case EKeys::F: return TEXT("F");
    case EKeys::G: return TEXT("G");
    case EKeys::H: return TEXT("H");
    case EKeys::I: return TEXT("I");
    case EKeys::J: return TEXT("J");
    case EKeys::K: return TEXT("K");
    case EKeys::L: return TEXT("L");
    case EKeys::M: return TEXT("M");
    case EKeys::N: return TEXT("N");
    case EKeys::O: return TEXT("O");
    case EKeys::P: return TEXT("P");
    case EKeys::Q: return TEXT("Q");
    case EKeys::R: return TEXT("R");
    case EKeys::S: return TEXT("S");
    case EKeys::T: return TEXT("T");
    case EKeys::U: return TEXT("U");
    case EKeys::V: return TEXT("V");
    case EKeys::W: return TEXT("W");
    case EKeys::X: return TEXT("X");
    case EKeys::Y: return TEXT("Y");
    case EKeys::Z: return TEXT("Z");

        // 숫자
    case EKeys::Zero: return TEXT("0");
    case EKeys::One: return TEXT("1");
    case EKeys::Two: return TEXT("2");
    case EKeys::Three: return TEXT("3");
    case EKeys::Four: return TEXT("4");
    case EKeys::Five: return TEXT("5");
    case EKeys::Six: return TEXT("6");
    case EKeys::Seven: return TEXT("7");
    case EKeys::Eight: return TEXT("8");
    case EKeys::Nine: return TEXT("9");

        // 특수 문자
    case EKeys::SpaceBar: return TEXT("Space");
    case EKeys::Tab: return TEXT("Tab");
    case EKeys::Enter: return TEXT("Enter");
    case EKeys::BackSpace: return TEXT("Backspace");
    case EKeys::Escape: return TEXT("Escape");

    case EKeys::Semicolon: return TEXT(";");
    case EKeys::Equals: return TEXT("=");
    case EKeys::Comma: return TEXT(",");
    case EKeys::Hyphen: return TEXT("-");
    case EKeys::Period: return TEXT(".");
    case EKeys::Slash: return TEXT("/");
    case EKeys::Tilde: return TEXT("~");
    case EKeys::LeftBracket: return TEXT("[");
    case EKeys::Backslash: return TEXT("\\");
    case EKeys::RightBracket: return TEXT("]");
    case EKeys::Apostrophe: return TEXT("'");

    case EKeys::RightMouseButton: return TEXT("RightMouseButton");
    case EKeys::LeftMouseButton: return TEXT("LeftMouseButton");

        // 기본값
    default:
        //assert(0);
        return TEXT("Invalid");
    }
}

// 알파벳은 대문자만 허용
EKeys::Type EKeys::FromString(const FString& KeyName)
{
    // 알파벳 (대문자만 허용)
    if (KeyName.Len() == 1)
    {
        TCHAR Char = KeyName[0];
        if (Char >= 'A' && Char <= 'Z')
            return static_cast<EKeys::Type>(EKeys::A + (Char - 'A'));
        if (Char >= '0' && Char <= '9')
            return static_cast<EKeys::Type>(EKeys::Zero + (Char - '0'));
        if (Char == ';') return EKeys::Semicolon;
        if (Char == '=') return EKeys::Equals;
        if (Char == ',') return EKeys::Comma;
        if (Char == '-') return EKeys::Hyphen;
        if (Char == '.') return EKeys::Period;
        if (Char == '/') return EKeys::Slash;
        if (Char == '~') return EKeys::Tilde;
        if (Char == '[') return EKeys::LeftBracket;
        if (Char == '\\') return EKeys::Backslash;
        if (Char == ']') return EKeys::RightBracket;
        if (Char == '\'') return EKeys::Apostrophe;
    }

    // 문자열 비교 (대소문자 구분 없음)
    if (KeyName.Equals(TEXT("Space"), ESearchCase::IgnoreCase)) return EKeys::SpaceBar;
    if (KeyName.Equals(TEXT("Tab"), ESearchCase::IgnoreCase)) return EKeys::Tab;
    if (KeyName.Equals(TEXT("Enter"), ESearchCase::IgnoreCase)) return EKeys::Enter;
    if (KeyName.Equals(TEXT("Backspace"), ESearchCase::IgnoreCase)) return EKeys::BackSpace;
    if (KeyName.Equals(TEXT("Escape"), ESearchCase::IgnoreCase)) return EKeys::Escape;

    // 마우스
    if (KeyName.Equals(TEXT("RightMouseButton"), ESearchCase::IgnoreCase)) return EKeys::RightMouseButton;
    if (KeyName.Equals(TEXT("LeftMouseButton"), ESearchCase::IgnoreCase)) return EKeys::LeftMouseButton;

    return EKeys::Invalid;
}

bool EKeys::IsKeyboardKey(EKeys::Type Key)
{
    return (Key >= EKeys::A && Key <= EKeys::Z) ||
        (Key >= EKeys::Zero && Key <= EKeys::Nine) ||
        Key == EKeys::SpaceBar || Key == EKeys::Tab ||
        Key == EKeys::Enter || Key == EKeys::BackSpace ||
        Key == EKeys::Escape || Key == EKeys::Semicolon ||
        Key == EKeys::Equals || Key == EKeys::Comma ||
        Key == EKeys::Hyphen || Key == EKeys::Period ||
        Key == EKeys::Slash || Key == EKeys::Tilde ||
        Key == EKeys::LeftBracket || Key == EKeys::Backslash ||
        Key == EKeys::RightBracket || Key == EKeys::Apostrophe;
}

bool EKeys::IsMouseKey(EKeys::Type Key)
{
    return Key == EKeys::LeftMouseButton || Key == EKeys::RightMouseButton ||
        Key == EKeys::MiddleMouseButton || Key == EKeys::ThumbMouseButton ||
        Key == EKeys::ThumbMouseButton2 || Key == EKeys::MouseX ||
        Key == EKeys::MouseY || Key == EKeys::Mouse2D ||
        Key == EKeys::MouseScrollUp || Key == EKeys::MouseScrollDown ||
        Key == EKeys::MouseWheelAxis;
}

EMouseButtons::Type EKeys::ToMouseButton(EKeys::Type Key)
{
    switch (Key)
    {
    case EKeys::LeftMouseButton:   return EMouseButtons::Type::Left;
    case EKeys::MiddleMouseButton: return EMouseButtons::Type::Middle;
    case EKeys::RightMouseButton:  return EMouseButtons::Type::Right;
    case EKeys::ThumbMouseButton:  return EMouseButtons::Type::Thumb01;
    case EKeys::ThumbMouseButton2: return EMouseButtons::Type::Thumb02;
    default:                       return EMouseButtons::Type::Invalid;
    }
}
