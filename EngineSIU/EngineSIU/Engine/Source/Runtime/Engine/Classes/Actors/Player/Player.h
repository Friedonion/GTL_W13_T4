#pragma once

#include "GameFramework/Character.h"

#include "PlayerDefine.h"

class USkeletalMeshComponent;
class UCameraComponent;
class UAnimSequence;
class GameObject;

// ACharacter를 상속받는 게임 내에서의 플레이어 캐릭터.
class APlayerCharacter : public ACharacter
{
    DECLARE_CLASS(APlayerCharacter, ACharacter)

public:
    APlayerCharacter();

    virtual void BeginPlay() override;

    virtual UObject* Duplicate(UObject* InOuter) override;
    virtual void Tick(float DeltaTime) override;

    virtual void RegisterLuaType(sol::state& Lua); // Lua에 클래스 등록해주는 함수.

    virtual bool BindSelfLuaProperties(); // LuaEnv에서 사용할 멤버 변수 등록 함수.

    void Punch();

    void Shoot();

    void SetPlayRate(float PlayRate);

    void SetWorldTickRate(float TickRate);

    bool IsPunching();
    
    bool IsShooting();

protected:
    USkeletalMeshComponent* LeftArm = nullptr;
    USkeletalMeshComponent* RightArm = nullptr;
    UAnimSequence* PunchAnim = nullptr;
    UAnimSequence* ShootAnim = nullptr;

    USceneComponent* Head = nullptr;

    UPROPERTY(
        EditAnywhere,
        bool,
        bAnimRestart,
        = true
    )
    
    UPROPERTY(
        EditAnywhere,
        float,
        DeltaTimeMultiplier,
        = 0.1f
    )
public:
    FVector GetHeadLocation() { return Head->GetComponentLocation(); }
    void SetHeadLocation(const FVector& Location) { Head->SetWorldLocation(Location); }

    void SetHeadRotation(const FRotator& Rotation) { Head->SetWorldRotation(Rotation); }
    FRotator GetHeadRotation() { return Head->GetComponentRotation(); }

private:
    bool bPunchingPending = false;
    bool bShootingPending = false;

    float bPunchingTimeLeft = 0.f; // 애니메이션과 맞추기 위함
    float bShootingTimeLeft = 0.f;

    UPROPERTY(EditAnywhere, float, bPunchingWaitTime, = 0.3f)
    UPROPERTY(EditAnywhere, float, bShootingWaitTime, = 0.3f)

    void ProcessAttack(float DeltaTime);

    void PunchInternal();
    void ShootInternal();
    
    bool bMoving = false;

private:

    void HandleCollision(GameObject* HitGameObject, AActor* SelfActor, AActor* OtherActor);
};
