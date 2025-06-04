#include "UncannyGameMode.h"
#include "LuaScripts/LuaUIManager.h"
#include "Developer/LuaUtils/LuaTextUI.h"
#include "Developer/LuaUtils/LuaImageUI.h"
#include "Developer/LuaUtils/LuaButtonUI.h"
#include "SoundManager.h"
#include "World/World.h"
#include "Engine/Contents/Objects/DamageCameraShake.h"
#include "Animation/AnimSoundNotify.h"
#include "Actors/DirectionalLightActor.h"

void AUncannyGameMode::BeginPlay()
{
    Super::BeginPlay();
}

void AUncannyGameMode::PostSpawnInitialize()
{
    Super::PostSpawnInitialize();

    UIManager = &LuaUIManager::Get();
    UIManager->DeleteUI(FName(*KillTextName));
    UIManager->DeleteUI(FName(*HPTextName));
    UIManager->DeleteUI(FName(*BulletTextName));

    int Width = UIManager->GetCanvasRectTransform().Size.X;
    int Height = UIManager->GetCanvasRectTransform().Size.Y;

    //// 타이틀 전체 화면 이미지
    UIManager->CreateImage(
        FName(*TitleImageName),
        RectTransform(0, 0, Width, Height, AnchorDirection::TopLeft),
        1,
        FName("Title"),
        FLinearColor(1, 1, 1, 1)
    );

    UIManager->CreateImage(
        FName(*SubUVImageName),
        RectTransform(-Width * 0.25, Height * 0.1, Width*0.5, Height*0.4, AnchorDirection::TopCenter),
        3,
        FName("TitleSubUV"), 
        FLinearColor(1, 1, 1, 1)
    );
    if (LuaImageUI* SubUVImage = UIManager->GetImageUI(*SubUVImageName))
    {
        SubUVImage->SetSubUVAnimation(5, 19, 0.05f, true);
        SubUVImage->PlaySubUV();
    }

    // GameStart 버튼 생성
    UIManager->CreateButton(
        "UI_GameStartButton",
        RectTransform(-128, -200, 256, 128, AnchorDirection::BottomCenter),
        2,
        "" // Lua 함수 안 씀
    );

    // 버튼 포인터 획득 후 설정
    if (LuaButtonUI* Btn = UIManager->GetButtonUI("UI_GameStartButton"))
    {
        FTexture* Tex = UIManager->GetTextureByName("Start");
        if (Tex)
            Btn->SetTexture((ImTextureID)(intptr_t)Tex->TextureSRV);

        Btn->SetOnClick([this]() {
            this->StartMatch();
            });
    }
    FMOD::Channel* channel = FSoundManager::GetInstance().PlaySound2D("BGM"); // 타이틀 BGM 재생
    FSoundManager::GetInstance().UpdateVolume(channel, 0.6f); // 볼륨 조절
    // 초기 HP 설정
    SetMaxHP(100);
    SetCurrentHP(100);
    SetBulletCount(30);
    KillCount = 0;
    bNoisePlaying = false;
    NoiseEffectElapsed = 0.f;
}


void AUncannyGameMode::StartMatch()
{
    Super::StartMatch();

    // 타이틀 이미지 제거
    UIManager->DeleteUI(FName(*TitleImageName));
    UIManager->DeleteUI("UI_GameStartButton");
    UIManager->DeleteUI(FName(*SubUVImageName));
    // 게임 UI 생성
    UIManager->CreateText(
        FName(*HPTextName),
        RectTransform(100, -40, 200, 30, AnchorDirection::BottomLeft),
        1,
        FString::Printf(TEXT("HP: %d / %d"), CurrentHP, MaxHP),
        FName("Default"),
        22,
        FLinearColor(1, 0, 0, 1)
    );

    UIManager->CreateText(
        FName(*KillTextName),
        RectTransform(-100, 70, 200, 30, AnchorDirection::TopCenter),
        1,
        FString::Printf(TEXT("Kills: %d"), KillCount),
        FName("Default"),
        22,
        FLinearColor(1, 1, 0, 1)
    );

    UIManager->CreateText(
        FName(*BulletTextName),
        RectTransform(-100, -40, 200, 30, AnchorDirection::BottomRight),
        1,
        FString::Printf(TEXT("Bullet: %d"), BulletCount),
        FName("Default"),
        22,
        FLinearColor(1, 1, 1, 1)
    );

    UIManager->CreateImage("TestImage2", RectTransform(-25, -25, 50, 50, AnchorDirection::MiddleCenter), 3, FName("Aim"), FLinearColor(1, 0, 0, 1));

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
            UIManager->DeleteUI("Noise");
            bNoisePlaying = false;
        }
    }

    //if (static_cast<int>(DeltaTime * 1024) % 4 == 1)
    //{

    //    for (ADirectionalLight* Actor : TObjectRange<ADirectionalLight>())
    //    {
    //        if (Actor->GetWorld()->WorldType != EWorldType::Editor)
    //        {
    //            LightActor = Actor;
    //            break;
    //        }
    //    }

    //    OriginalIntensity = LightActor->GetIntensity();
    //    LightActor->SetIntensity(100);
    //    LighteningCounter = 5;
    //}

    //if (LighteningCounter > 0)
    //{
    //    LighteningCounter--;
    //    if (LighteningCounter == 0)
    //    {
    //        LightActor->SetIntensity(OriginalIntensity);
    //    }
    //}
}


void AUncannyGameMode::SetCurrentHP(int32 NewHP)
{
    CurrentHP = FMath::Clamp(NewHP, 0, MaxHP);
    UpdateUI();
    if(CurrentHP <= 0)
    {
        OnDeath();
    }
}

void AUncannyGameMode::OnPlayerHit(float Damage)
{
    CurrentHP -= FMath::Max(0, (int32)Damage);
    UpdateUI();
    GetWorld()->GetPlayerController()->ClientStartCameraShake(UDamageCameraShake::StaticClass());
    if (CurrentHP <= 0)
    {
        OnDeath();
    }
    else
    {
        PlayHitNoiseEffect();
    }
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
    if (ShootNotify)
    {
        if (BulletCount <= 0)
        {
             ShootNotify->SetSoundName(FName("Empty"));
        }
        else
        {
            ShootNotify->SetSoundName(FName("Pistol"));
        }
    }
    BulletCount = FMath::Max(0, NewCount);
    UpdateUI();

    // 0이 들어온 것은 1발이 남았을 때 발사한것 -> 0까지는 소리 바꾸지않음.
}

void AUncannyGameMode::UpdateUI()
{
    if (LuaTextUI* HPText = UIManager->GetTextUI(FName(*HPTextName)))
    {
        FString Text = FString::Printf(TEXT("HP: %d / %d"), CurrentHP, MaxHP);
        HPText->SetText(Text);
    }

    if (LuaTextUI* BulletText = UIManager->GetTextUI(FName(*BulletTextName)))
    {
        FString Text = FString::Printf(TEXT("Bullet: %d"), BulletCount);
        BulletText->SetText(Text);
    }

    if (LuaTextUI* KillText = UIManager->GetTextUI(FName(*KillTextName)))
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

    int Width = UIManager->GetCanvasRectTransform().Size.X;
    int Height = UIManager->GetCanvasRectTransform().Size.Y;

    UIManager->CreateImage("Noise",
        RectTransform(-Width / 2, -Height / 2, Width, Height, AnchorDirection::MiddleCenter),
        3,
        FName("Noise"),
        FLinearColor(1, 0, 0, 1)
    );

    if (LuaImageUI* NoiseImage = UIManager->GetImageUI("Noise"))
    {
        NoiseImage->SetSubUVAnimation(5, 3, 0.05f, true); 
        NoiseImage->PlaySubUV();
    }

    bNoisePlaying = true;
    NoiseEffectElapsed = 0.f;
}

void AUncannyGameMode::OnDeath()
{
    APlayerController* Controller = GetWorld()->GetPlayerController();
    Controller->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, 10.f, FLinearColor::Black, false);

    Controller->UnPossess();
}


