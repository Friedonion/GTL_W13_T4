#include "GameManager.h"
#include "LuaScripts/LuaUIManager.h"
#include "Developer/LuaUtils/LuaTextUI.h"
#include "Developer/LuaUtils/LuaImageUI.h"
#include "Developer/LuaUtils/LuaButtonUI.h"
#include "Developer/LuaUtils/LuaUI.h"
#include "Launch/EngineLoop.h"
#include "World/World.h"

AGameManager::AGameManager()
// 'LuaUIManager::Get()'은 LuaUIManager 싱글턴 인스턴스(전역으로 단 하나만 존재하는 객체)를 가져오는 함수
    : UIManagerRef(LuaUIManager::Get())
    , EnemiesKilledCount(0)
    , CurrentAmmo(30), MaxAmmo(30) // 시작 총알은 30/30
    , CurrentHP(100), MaxHP(100)   // 시작 체력은 100/100
    // UI 업데이트 플래그를 모두 false로 초기화합니다.
    , bNeedsUIUpdate_Kills(false)
    , bNeedsUIUpdate_Ammo(false)
    , bNeedsUIUpdate_HP(false)
    // SubUV 애니메이션 관련 변수들을 초기화합니다.
    , SubUVFrameTime(0.05f) // 0.05초마다 프레임 전환
    , SubUVTotalFrames(36)  // 6x6 스프라이트 시트 (총 36프레임)
    // 사용할 SubUV 텍스처 이름 지정 (LuaUIManager::GenerateResource()에서 로드되어야 합니다)
    , HPSubUVTextureName(FName("HP_SubUV_Effect")) // 예시 이름, 실제 텍스처 파일과 일치해야 합니다.
    , AmmoSubUVTextureName(FName("Ammo_SubUV_Effect")) // 예시 이름
{    
    UIManagerRef.CreateText(
        FName("HPText"),
        RectTransform(10.f, -50.f, 200.f, 50.f, AnchorDirection::BottomLeft),
        100,
        FString("HP: 100/100"),
        FName("Default"),
        30.f,
        FLinearColor(1.f, 1.f, 1.f, 1.f)
    );
    HPTextUI = UIManagerRef.GetTextUI(FName("HPText"));

    UIManagerRef.CreateImage(
        FName("HPSubUVImage"),                                  // 이 UI 요소의 고유한 이름
        RectTransform(0.f, -10.f, 100.f, 100.f, AnchorDirection::BottomLeft), // 위치, 크기, 기준점
        99,                                                     // SortOrder (텍스트보다 아래에 그려지도록 99)
        HPSubUVTextureName,                                     // 사용할 텍스처 이름
        FLinearColor(1.f, 1.f, 1.f, 1.f)                        // 이미지 색상 (흰색)
    );
    // 생성된 HPSubUVImage UI에 대한 포인터를 저장합니다.
    HPSubUVImageUI = UIManagerRef.GetImageUI(FName("HPSubUVImage"));
    // 포인터가 유효한지 확인하고 SubUV 애니메이션을 설정 및 재생합니다.
    if (HPSubUVImageUI)
    {
        HPSubUVImageUI->SetSubUVAnimation(6, 6, SubUVFrameTime, true); // 6x6 그리드, 0.05초 간격, 반복 재생
        HPSubUVImageUI->PlaySubUV(); // 애니메이션 재생 시작
    }

}

void AGameManager::BeginPlay()
{
    //Super::BeginPlay();

    UpdateHPUI();

    SetMaxHP(100);

    AddHP(0);
}

void AGameManager::Tick(float DeltaTime)
{
    //Super::Tick(DeltaTime);

    UIManagerRef.ActualDeleteUIs();

    if (bNeedsUIUpdate_HP)
    {
        UpdateHPUI();
        bNeedsUIUpdate_HP = false;
    }

    // LuaUIManager 에게 매 프레임마다 UI 그려줘 명령
    UIManagerRef.DrawLuaUIs();

    // 체력 테스트용 
    static float TestDamageTimer = 0.f;
    TestDamageTimer += DeltaTime; 
    if (TestDamageTimer >= 1.0f) // 1초마다
    {
        TakeDamage(1); // 체력 1 감소
        TestDamageTimer = 0.f; // 타이머 리셋
    }
}

void AGameManager::AddHP(int32 Amount)
{
    CurrentHP += Amount;

    if (CurrentHP > MaxHP)
    {
        CurrentHP = MaxHP;
    }
    bNeedsUIUpdate_HP = true; 
}

void AGameManager::TakeDamage(int32 Amount)
{
    CurrentHP -= Amount;
    if (CurrentHP < 0)
    {
        CurrentHP = 0;
    }
    // TODO : 체력 0 일때 gameover 로직
    bNeedsUIUpdate_HP = true;
}

void AGameManager::SetMaxHP(int32 Amount)
{
    MaxHP = Amount;
    if (CurrentHP > MaxHP)
    {
        CurrentHP = MaxHP;
    }
    bNeedsUIUpdate_HP = true;
}

void AGameManager::UpdateHPUI()
{
    if (HPTextUI)
    {
        FString HealthText = FString::Printf(TEXT("HP: %d / %d"), CurrentHP, MaxHP);

        HPTextUI->SetText(HealthText);
    }
}

void AGameManager::ChangeAmmo(int32 Amount)
{
}

void AGameManager::SetMaxAmmo(int32 Amount)
{
}

void AGameManager::KillEnemy()
{
}

void AGameManager::UpdateEnemiesKilledUI()
{
}

void AGameManager::UpdateAmmoUI()
{
}
