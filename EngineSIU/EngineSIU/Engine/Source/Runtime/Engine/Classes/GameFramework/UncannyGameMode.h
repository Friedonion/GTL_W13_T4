#pragma once
#include "GameMode.h"

class LuaUIManager;
class UAnimSoundNotify;

class AUncannyGameMode : public AGameMode
{
    DECLARE_CLASS(AUncannyGameMode, AGameMode)

public:
    AUncannyGameMode() = default;
    virtual ~AUncannyGameMode() override = default;

	virtual void BeginPlay();

    virtual void PostSpawnInitialize() override;
    virtual void Tick(float DeltaTime) override;

    void SetCurrentHP(int32 NewHP);
    int32 GetCurrentHP() const { return CurrentHP; }
    void OnPlayerHit(float Damage = 10.f);
    void SetMaxHP(int32 NewMaxHP);
    void SetBulletCount(int32 NewCount);
    int32 GetBulletCount() const { return BulletCount; }
    void AddKill();
    void PlayHitNoiseEffect();
    void OnDeath();

    virtual void StartMatch() override;

    UAnimSoundNotify* ShootNotify = nullptr;

protected:
    void UpdateUI();

private:
    int32 CurrentHP = 100;
    int32 MaxHP = 100;
    int32 BulletCount = 30;
    int32 KillCount = 0;

    bool bNoisePlaying = false;
    float NoiseEffectElapsed = 0.f;


    FString KillTextName = TEXT("UI_KillText");
    FString HPTextName = "UI_HPText";
    FString BulletTextName = "UI_BulletText";
    FString TitleImageName = "UI_TitleImage";
    FString SubUVImageName = "UI_SubUVImage";
    
    LuaUIManager* UIManager = nullptr;

	class ADirectionalLight* LightActor;
	int LighteningCounter = 0;
	float OriginalIntensity;
};
