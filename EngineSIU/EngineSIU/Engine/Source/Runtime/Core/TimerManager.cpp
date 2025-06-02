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

    ActiveTimers.Remove(Handle);
}

void FTimerManager::PauseTimer(FTimerHandle Handle)
{
    if (!Handle.IsValid()) return;

    FTimerData* Timer = ActiveTimers.Find(Handle);
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
    if (!Handle.IsValid()) return false;

    const FTimerData* Timer = ActiveTimers.Find(Handle);
    if (Timer)
    {
        return Timer->bIsPaused;
    }
    return false;
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
