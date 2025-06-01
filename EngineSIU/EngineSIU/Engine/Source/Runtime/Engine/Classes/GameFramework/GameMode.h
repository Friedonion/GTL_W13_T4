#pragma once
#include "Actor.h"
#include "Delegates/DelegateCombination.h"

DECLARE_MULTICAST_DELEGATE(FOnGameInit);
DECLARE_MULTICAST_DELEGATE(FOnGameStart);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnGameEnd, bool);

class UCameraComponent;

struct FGameInfo
{
    float TotalGameTime = 0.0f;
    float ElapsedGameTime = 0.0f;
    uint32 CoinScore = 0;
};

class AGameMode : public AActor
{
    DECLARE_CLASS(AGameMode, AActor)
    
public:
    AGameMode() = default;
    virtual ~AGameMode() override = default;

    virtual void PostSpawnInitialize() override;
    
    void InitializeComponent();
    virtual UObject* Duplicate(UObject* InOuter) override;

    //virtual void BeginPlay() override;
    //virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // 게임 모드 초기화
    virtual void InitGame();

    // 게임 시작
    virtual void StartMatch();

    // 게임 종료
    virtual void EndMatch(bool bIsWin);

    virtual void Tick(float DeltaTime) override;

    void Reset();

    FOnGameInit OnGameInit;
    FOnGameStart OnGameStart;
    FOnGameEnd OnGameEnd;

    FGameInfo GameInfo;
    
private:
    bool bGameRunning = false; // 내부 
    bool bGameEnded = true;

    float LogTimer = 0.f;
    float LogInterval = 1.f;  // 1초마다 로그
};

