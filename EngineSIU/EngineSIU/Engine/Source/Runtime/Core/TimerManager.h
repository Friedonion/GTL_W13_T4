#pragma once

#include "Core/TimerData.h"
#include "Container/Array.h"
#include "Container/Map.h"

#include <atomic>

class FTimerManager
{
private:
    TMap<FTimerHandle, FTimerData> ActiveTimers;
    std::atomic<uint64> NextTimerID = 1;                // 고유 ID 생성을 위한 카운터

    void ExecuteTimer(FTimerData& Timer);

public:
    FTimerManager();
    ~FTimerManager() = default; // 실제로는 Timers 벡터 내의 Callback 정리 등이 필요할 수 있음

    void Tick(float DeltaTime);

    FTimerHandle SetTimer(float Rate, bool bLoop, std::function<void()> Callback);

    template <typename UserClass>
    FTimerHandle SetTimer(UserClass* Object, void (UserClass::* Method)(), float Rate, bool bLoop)
    {
        if (!Object || !Method || Rate <= 0.0f)
        {
            return FTimerHandle();
        }

        std::function<void()> Callback = [Object, Method]() {
            // 객체 유효성 검사 (약한 참조 시스템이 없다면 수동으로)
            // if (Object가 유효한지 확인하는 방법) {
            (Object->*Method)();
            // }
            };

        FTimerHandle NewHandle(GenerateNewTimerID());
        ActiveTimers.Emplace(NewHandle, FTimerData(NewHandle, Rate, bLoop, Callback)); // UserObject 미사용 시

        return NewHandle;
    }

    // 타이머 제거
    void ClearTimer(FTimerHandle Handle);

    // 타이머 일시 정지/재개 (선택적 기능)
    void PauseTimer(FTimerHandle Handle);
    void UnPauseTimer(FTimerHandle Handle);
    bool IsTimerPaused(FTimerHandle Handle) const;
    bool IsTimerActive(FTimerHandle Handle) const; // 유효하고 일시정지되지 않았는지

private:
    uint64 GenerateNewTimerID();
};

