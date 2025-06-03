#pragma once
#include "GameMode.h"

class AUncannyGameMode : public AGameMode
{
    DECLARE_CLASS(AUncannyGameMode, AGameMode)

public:
    AUncannyGameMode() = default;
    virtual ~AUncannyGameMode() override = default;

    virtual void PostSpawnInitialize() override;
    virtual void Tick(float DeltaTime) override;

    void SetCurrentHP(int32 NewHP);
    void SetMaxHP(int32 NewMaxHP);
    void SetBulletCount(int32 NewCount);
    void AddKill();

protected:
    void UpdateUI();

private:
    int32 CurrentHP = 100;
    int32 MaxHP = 100;
    int32 BulletCount = 30;
    int32 KillCount = 0;

    FString KillTextName = TEXT("UI_KillText");
    FString HPTextName = "UI_HPText";
    FString BulletTextName = "UI_BulletText";
};
