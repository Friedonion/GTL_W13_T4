#pragma once

#include "GameFramework/Character.h"

class USkeletalMeshComponent;
class UCameraComponent;

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
    
protected:
    USkeletalMeshComponent* LeftArm = nullptr;
    USkeletalMeshComponent* RightArm = nullptr;
};
