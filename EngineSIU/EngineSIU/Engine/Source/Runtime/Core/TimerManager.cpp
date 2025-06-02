#include "TimerManager.h"

uint64 FTimerManager::GenerateNewTimerID()
{
    uint64 Result = NextTimerID.fetch_add(1, std::memory_order_relaxed);
    if (Result == 0)
    {
        Result = NextTimerID.fetch_add(1, std::memory_order_relaxed);
    }
    return Result;
}

void FTimerManager::Tick(float DeltaTime)
{
    // TMap을 순회할 때는 이터레이터를 사용합니다.
    // 순회 중 맵에서 요소를 제거해야 할 경우, 제거 후 이터레이터가 무효화될 수 있으므로 주의해야 합니다.
    // 일반적인 방법은 제거할 핸들들을 먼저 수집하고 루프 후에 한꺼번에 제거하거나,
    // C++17의 경우 map::erase(iterator)가 다음 유효한 이터레이터를 반환하는 것을 활용할 수 있습니다.
    // 또는, 제거 시 해당 이터레이터만 무효화되므로, 올바르게 다음 이터레이터로 이동하면 됩니다.

    TArray<FTimerHandle> HandlesToRemove;

    for (auto& Pair : ActiveTimers)
    {
        FTimerHandle Handle = Pair.Key;
        FTimerData& Timer = Pair.Value;

        if (!Timer.Handle.IsValid() || Timer.bIsPaused)
        {
            continue;
        }

        Timer.TimeRemaining -= DeltaTime;

        if (Timer.TimeRemaining <= 0.0f)
        {
            ExecuteTimer(Timer);

            if (Timer.bLoop)
            {
                Timer.TimeRemaining += Timer.Rate;
            }
            else
            {
                HandlesToRemove.Add(Handle);
            }
        }
    }

    for (const FTimerHandle& HandleToRemove : HandlesToRemove)
    {
        ActiveTimers.Remove(HandleToRemove);
    }
}

void FTimerManager::ExecuteTimer(FTimerData& Timer)
{
    if (Timer.Callback)
    {
        Timer.Callback();
    }
}

FTimerManager::FTimerManager()
{
}

FTimerHandle FTimerManager::SetTimer(float Rate, bool bLoop, std::function<void()> Callback)
{
    if (Rate <= 0.0f || !Callback)
    {
        return FTimerHandle();
    }

    FTimerHandle NewHandle(GenerateNewTimerID());

    ActiveTimers.Emplace(NewHandle, FTimerData(NewHandle, Rate, bLoop, Callback));

    return NewHandle;
}

void FTimerManager::ClearTimer(FTimerHandle Handle)
{
    if (!Handle.IsValid()) return;

    ActiveTimers.Remove(Handle); // TMap의 Remove 사용
}

void FTimerManager::PauseTimer(FTimerHandle Handle)
{
    if (!Handle.IsValid()) return;

    FTimerData* Timer = ActiveTimers.Find(Handle); // ValueType에 대한 포인터 반환
    if (Timer)
    {
        Timer->bIsPaused = true;
    }
}

void FTimerManager::UnPauseTimer(FTimerHandle Handle)
{
    if (!Handle.IsValid()) return;

    FTimerData* Timer = ActiveTimers.Find(Handle);
    if (Timer)
    {
        Timer->bIsPaused = false;
    }
}

bool FTimerManager::IsTimerPaused(FTimerHandle Handle) const
{
    if (!Handle.IsValid()) return false; // 또는 true (유효하지 않으면 일시정지 상태로 간주?)

    const FTimerData* Timer = ActiveTimers.Find(Handle); // const 버전의 Find 사용
    if (Timer)
    {
        return Timer->bIsPaused;
    }
    return false; // 또는 true
}

bool FTimerManager::IsTimerActive(FTimerHandle Handle) const
{
    if (!Handle.IsValid()) return false;

    const FTimerData* Timer = ActiveTimers.Find(Handle);
    if (Timer)
    {
        return !Timer->bIsPaused; // Handle이 유효하고, 타이머를 찾았고, 일시정지되지 않았음
    }
    return false;
}
