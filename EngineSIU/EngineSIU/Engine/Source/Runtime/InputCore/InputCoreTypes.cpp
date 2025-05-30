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
        assert(0);
        return EKeys::Invalid;
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

        // 기본값
    default:
        assert(0);
        return TEXT("Invalid");
    }
}
