#include "UncannyGameMode.h"
#include "LuaScripts/LuaUIManager.h"
#include "Developer/LuaUtils/LuaTextUI.h"
#include "Developer/LuaUtils/LuaImageUI.h"
#include "Developer/LuaUtils/LuaButtonUI.h"

void AUncannyGameMode::PostSpawnInitialize()
{
    Super::PostSpawnInitialize();

    LuaUIManager::Get().DeleteUI(FName(*KillTextName));
    LuaUIManager::Get().DeleteUI(FName(*HPTextName));
    LuaUIManager::Get().DeleteUI(FName(*BulletTextName));

    int Width = LuaUIManager::Get().GetCanvasRectTransform().Size.X;
    int Height = LuaUIManager::Get().GetCanvasRectTransform().Size.Y;

    //// 타이틀 전체 화면 이미지
    LuaUIManager::Get().CreateImage(
        FName(*TitleImageName),
        RectTransform(0, 0, Width, Height, AnchorDirection::TopLeft),
        1,
        FName("Title"),
        FLinearColor(1, 1, 1, 1)
    );

    // GameStart 버튼 생성
    LuaUIManager::Get().CreateButton(
        "UI_GameStartButton",
        RectTransform(-128, -200, 256, 128, AnchorDirection::BottomCenter),
        2,
        "" // Lua 함수 안 씀
    );

    // 버튼 포인터 획득 후 설정
    if (LuaButtonUI* Btn = LuaUIManager::Get().GetButtonUI("UI_GameStartButton"))
    {
        FTexture* Tex = LuaUIManager::Get().GetTextureByName("Start");
        if (Tex)
            Btn->SetTexture((ImTextureID)(intptr_t)Tex->TextureSRV);

        Btn->SetOnClick([this]() {
            this->StartMatch();
            });
    }
}


void AUncannyGameMode::StartMatch()
{
    Super::StartMatch();

    // 타이틀 이미지 제거
    LuaUIManager::Get().DeleteUI(FName(*TitleImageName));
    LuaUIManager::Get().DeleteUI("UI_GameStartButton");

    // 게임 UI 생성
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

    LuaUIManager::Get().CreateImage("TestImage2", RectTransform(-25, -25, 50, 50, AnchorDirection::MiddleCenter), 3, FName("Aim"), FLinearColor(1, 0, 0, 1));

    UpdateUI();
}


void AUncannyGameMode::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bNoisePlaying)
    {
        NoiseEffectElapsed += DeltaTime;
        if (NoiseEffectElapsed >= 0.15f) // 약 15프레임 * 0.05초
        {
            LuaUIManager::Get().DeleteUI("Noise");
            bNoisePlaying = false;
        }
    }
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

void AUncannyGameMode::PlayHitNoiseEffect()
{
    if (bNoisePlaying) return;

    int Width = LuaUIManager::Get().GetCanvasRectTransform().Size.X;
    int Height = LuaUIManager::Get().GetCanvasRectTransform().Size.Y;

    LuaUIManager::Get().CreateImage("Noise",
        RectTransform(-Width / 2, -Height / 2, Width, Height, AnchorDirection::MiddleCenter),
        3,
        FName("Noise"),
        FLinearColor(1, 0, 0, 1)
    );

    if (LuaImageUI* NoiseImage = LuaUIManager::Get().GetImageUI("Noise"))
    {
        NoiseImage->SetSubUVAnimation(5, 3, 0.05f, true); 
        NoiseImage->PlaySubUV();
    }

    bNoisePlaying = true;
    NoiseEffectElapsed = 0.f;
}


