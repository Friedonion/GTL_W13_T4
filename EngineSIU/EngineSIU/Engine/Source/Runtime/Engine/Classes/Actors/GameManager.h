#pragma once
#include "GameFramework/Actor.h"
#include "UObject/NameTypes.h"
#include "Container/String.h"
#include "Math/Color.h"

class LuaUIManager;
class LuaTextUI;
class LuaImageUI;

class AGameManager :   public AActor
{
    DECLARE_CLASS(AGameManager, AActor)
public:
   
    AGameManager();
    virtual ~AGameManager() = default;

    // AActor 의 생명주기 함수 override
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // 게임 상태 변수들
    int32 EnemiesKilledCount;
    int32 CurrentAmmo;
    int32 MaxAmmo;
    int32 CurrentHP;
    int32 MaxHP;

    // 게임 상태 변경 함수
    void AddHP(int32 Amount);
    void TakeDamage(int32 Amount);
    void SetMaxHP(int32 Amount);

    void UpdateHPUI();

    void ChangeAmmo(int32 Amount);
    void SetMaxAmmo(int32 Amount);

    void KillEnemy();
    // UI 업데이트 함수
    void UpdateEnemiesKilledUI();
    void UpdateAmmoUI();

private:
    // LuaManager 를 통해 생성된 UI 요소들에 대한 포인터
    LuaTextUI* EnemieKilledTextUI = nullptr;
    LuaTextUI* AmmoTextUI = nullptr;
    LuaTextUI* HPTextUI = nullptr;
    LuaImageUI* HPSubUVImageUI = nullptr;
    LuaImageUI* AmmoSubUVImageUI = nullptr;

    // LuaUIManager 싱글턴 인스턴스에 대한 참조
    LuaUIManager& UIManagerRef;

    // UI 업데이트가 필요한지 나타내는 플래그
    bool bNeedsUIUpdate_Kills = false;
    bool bNeedsUIUpdate_Ammo = false;
    bool bNeedsUIUpdate_HP = false;

    // SubUV 애니메이션 관련
    float SubUVFrameTime = 0.05f; // SubUV 애니메이션 프레임 간격 (초)
    int32 SubUVTotalFrames = 36; // SubUV 스프라이트 시트의 총 프레임 수 (예: 6x6 그리드 = 36개 프레임)

    FName HPSubUVTextureName = FName("HP_SubUV_Effect");
    FName AmmoSubUVTextureName = FName("Ammo_SubUV_Effect");
};

