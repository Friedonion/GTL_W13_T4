#include "UncannyGameMode.h"
#include "LuaScripts/LuaUIManager.h"
#include "Developer/LuaUtils/LuaTextUI.h"

void AUncannyGameMode::PostSpawnInitialize()
{
    Super::PostSpawnInitialize();

    LuaUIManager::Get().CreateText(
        FName(*HPTextName),
        RectTransform(10, -10, 200, 30, AnchorDirection::BottomLeft),
        1,
        FString::Printf(TEXT("HP: %d / %d"), CurrentHP, MaxHP),
        FName("Default"),
        22,
        FLinearColor(1, 0, 0, 1)
    );

    LuaUIManager::Get().CreateText(
        FName(*KillTextName),
        RectTransform(10, 70, 200, 30, AnchorDirection::TopCenter),
        1,
        FString::Printf(TEXT("Kills: %d"), KillCount),
        FName("Default"),
        22,
        FLinearColor(1, 1, 0, 1)
    );


    LuaUIManager::Get().CreateText(
        FName(*BulletTextName),
        RectTransform(-200, -10, 200, 30, AnchorDirection::BottomRight),
        1,
        FString::Printf(TEXT("Bullet: %d"), BulletCount),
        FName("Default"),
        22,
        FLinearColor(1, 1, 1, 1)
    );

    UpdateUI();
}

void AUncannyGameMode::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AUncannyGameMode::SetCurrentHP(int32 NewHP)
{
    CurrentHP = FMath::Clamp(NewHP, 0, MaxHP);
    UpdateUI();
}

void AUncannyGameMode::SetMaxHP(int32 NewMaxHP)
{
    MaxHP = FMath::Max(1, NewMaxHP);
    if (CurrentHP > MaxHP)
        CurrentHP = MaxHP;
    UpdateUI();
}

void AUncannyGameMode::SetBulletCount(int32 NewCount)
{
    BulletCount = FMath::Max(0, NewCount);
    UpdateUI();
}

void AUncannyGameMode::UpdateUI()
{
    if (LuaTextUI* HPText = LuaUIManager::Get().GetTextUI(FName(*HPTextName)))
    {
        FString Text = FString::Printf(TEXT("HP: %d / %d"), CurrentHP, MaxHP);
        HPText->SetText(Text);
    }

    if (LuaTextUI* BulletText = LuaUIManager::Get().GetTextUI(FName(*BulletTextName)))
    {
        FString Text = FString::Printf(TEXT("Bullet: %d"), BulletCount);
        BulletText->SetText(Text);
    }

    if (LuaTextUI* KillText = LuaUIManager::Get().GetTextUI(FName(*KillTextName)))
    {
        FString Text = FString::Printf(TEXT("Kills: %d"), KillCount);
        KillText->SetText(Text);
    }

}

void AUncannyGameMode::AddKill()
{
    KillCount++;
    UpdateUI();
}

