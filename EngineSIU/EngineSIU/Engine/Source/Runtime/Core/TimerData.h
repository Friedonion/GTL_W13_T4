#pragma once

#include <functional>
#include "Core/HAL/PlatformType.h"

// 타이머 핸들을 위한 간단한 ID (Delegate.h의 FDelegateHandle과 유사하게 만들거나, 단순 ID 사용)
struct FTimerHandle
{
    uint64 ID = 0;

    FTimerHandle() = default;
    explicit FTimerHandle(uint64 InID) : ID(InID) {}

    bool IsValid() const { return ID != 0; }
    void Invalidate() { ID = 0; }

    bool operator==(const FTimerHandle& Other) const { return ID == Other.ID; }
    bool operator!=(const FTimerHandle& Other) const { return ID != Other.ID; }
};

namespace std
{
    template <>
    struct hash<FTimerHandle>
    {
        size_t operator()(const FTimerHandle& Handle) const noexcept
        {
            return hash<uint64>()(Handle.ID);
        }
    };
}


struct FTimerData
{
    FTimerHandle Handle;                         // 타이머 식별자
    float Rate;                                  // 호출 간격 (초)
    bool bLoop;                                  // 반복 여부
    float TimeRemaining;                         // 다음 호출까지 남은 시간
    std::function<void()> Callback;              // 호출될 함수 (람다 또는 바인딩된 함수)
    bool bIsPaused = false;                      // 일시 정지 여부
    void* UserObject = nullptr;                  // (선택 사항) UObject 스타일의 약한 참조를 위해 객체 포인터 저장
    // TWeakObjectPtr 같은 것을 사용하려면 해당 시스템 필요

    FTimerData(FTimerHandle InHandle, float InRate, bool InbLoop, std::function<void()> InCallback, void* InUserObject = nullptr)
        : Handle(InHandle), 
        Rate(InRate), 
        bLoop(InbLoop), 
        TimeRemaining(InRate),
        Callback(InCallback), 
        UserObject(InUserObject), 
        bIsPaused(false) 
    {    }

    FTimerData() : Rate(0), bLoop(false), TimeRemaining(0), Callback(nullptr), UserObject(nullptr), bIsPaused(false) {}
};
