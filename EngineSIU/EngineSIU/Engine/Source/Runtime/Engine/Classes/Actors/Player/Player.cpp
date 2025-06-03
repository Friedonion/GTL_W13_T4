#include "Player.h"
#include "Engine/Engine.h"
#include "World/World.h"
#include "Engine/SkeletalMesh.h"
#include "Lua/LuaScriptComponent.h"
#include "Lua/LuaScriptManager.h"
#include "Lua/LuaUtils/LuaTypeMacros.h"
#include "Classes/Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimSequence.h"
#include "EngineLoop.h"
#include "Actors/Fist.h"
#include "Engine/Classes/Animation/AnimTypes.h"
#include "Animation/AnimSoundNotify.h"

APlayerCharacter::APlayerCharacter()
    : ACharacter()
{
    Head = AddComponent<USceneComponent>(FName("Head"));
    LeftArm = AddComponent<USkeletalMeshComponent>(FName("LeftArm"));
    RightArm = AddComponent<USkeletalMeshComponent>(FName("RightArm"));
    
    Head->SetupAttachment(Super::Mesh);
    LeftArm->SetupAttachment(Head);
    RightArm->SetupAttachment(Head);
}

void APlayerCharacter::BeginPlay()
{
    Super::BeginPlay();

    RegisterLuaType(FLuaScriptManager::Get().GetLua());

    sol::state& Lua = FLuaScriptManager::Get().GetLua();

    // Lua -> C++로 연결(아직 호출은 안함)
    Lua.set_function("RegisterKeyCallback",
        [](const std::string& Key, const std::function<void(float)>& Callback)
        {
            GEngine->ActiveWorld->GetPlayerController()->BindAction(FString(Key), Callback);
        }
    );

    Lua.set_function("RegisterMouseMoveCallback",
        [](const std::function<void(float, float)>& Callback)
        {
            GEngine->ActiveWorld->GetPlayerController()->BindMouseMove(Callback);
        }
    );


    for (USkeletalMeshComponent* SkelComp : GetComponentsByClass<USkeletalMeshComponent>())
    {
        SkelComp->BindAnimScriptInstance(this);
    }

    PunchAnim = Cast<UAnimSequence>(LeftArm->GetAnimation());
    ShootAnim = Cast<UAnimSequence>(RightArm->GetAnimation());

    int32 NewTrackIdx = INDEX_NONE;

    if (PunchAnim && ShootAnim)
    {
        PunchAnim->AddNotifyTrack("NewTrackName", NewTrackIdx);
        ShootAnim->AddNotifyTrack("NewTrackName", NewTrackIdx);

        // Punch Notify
        {
            int32 NotifyIndex = 0;
            if (!PunchAnim->GetNotifyEvent(NotifyIndex))
                PunchAnim->AddNotifyEvent(0, 0.168f, 0, "PunchNotify", NotifyIndex);

            if (FAnimNotifyEvent* NotifyEvent = PunchAnim->GetNotifyEvent(NotifyIndex))
            {
                auto* Notify = FObjectFactory::ConstructObject<UAnimSoundNotify>(nullptr);
                Notify->SetSoundName(FName("Punch"));
                NotifyEvent->SetAnimNotify(Notify);
            }
        }

        // Shoot Notify
        {
            int32 NotifyIndex = 0;
            if (!ShootAnim->GetNotifyEvent(NotifyIndex))
                ShootAnim->AddNotifyEvent(0, 0.168f, 0, "ShootNotify", NotifyIndex);

            if (FAnimNotifyEvent* NotifyEvent = ShootAnim->GetNotifyEvent(NotifyIndex))
            {
                auto* Notify = FObjectFactory::ConstructObject<UAnimSoundNotify>(nullptr);
                Notify->SetSoundName(FName("Pistol"));
                NotifyEvent->SetAnimNotify(Notify);
            }
        }
    }


    // C++코드를 호출
    LuaScriptComponent->ActivateFunction("InitializeCallback");

    // 현재 각 팔에 할당되어있는 애니메이션의 시간을 interval로 지정
    if (LeftArm)
    {
        //LeftArm->AnimClass->asset
    }


}

UObject* APlayerCharacter::Duplicate(UObject* InOuter)
{
    ThisClass* NewActor = Cast<ThisClass>(Super::Duplicate(InOuter));

    //NewActor->LeftArm = Cast<USkeletalMeshComponent>(LeftArm->Duplicate(NewActor));
    //NewActor->RightArm = Cast<USkeletalMeshComponent>(RightArm->Duplicate(NewActor));
    // 위치 기반으로 연결

    for (USkeletalMeshComponent* SkelComp : NewActor->GetComponentsByClass<USkeletalMeshComponent>())
    {
        if (SkelComp->GetName() == LeftArm->GetName())
        {
            NewActor->LeftArm = SkelComp;
        }
        else if (SkelComp->GetName() == RightArm->GetName())
        {
            NewActor->RightArm = SkelComp;
        }
    }

    for (USceneComponent* Comp : NewActor->GetComponentsByClass<USceneComponent>())
    {
        if (Comp->GetName() == Head->GetName())
        {
            NewActor->Head = Comp;
        }
    }


    return NewActor;
}

void APlayerCharacter::Tick(float DeltaTime)
{
    AActor::Tick(DeltaTime);

    if (IsPunching())
    {
        this->SetWorldTickRate(1);
    }
    else if (IsShooting())
    {
        this->SetWorldTickRate(1);
    }
    else
    {
        this->SetWorldTickRate(0.3);
    }
}

void APlayerCharacter::RegisterLuaType(sol::state& Lua)
{
    DEFINE_LUA_TYPE_WITH_PARENT(APlayerCharacter, (sol::bases<AActor, APawn, ACharacter>()),
        "State", &APlayerCharacter::State,
        "Punch", &APlayerCharacter::Punch,
        "Shoot", &APlayerCharacter::Shoot,
        "SetPlayRate", &APlayerCharacter::SetPlayRate,
        "SetWorldTickRate", &APlayerCharacter::SetWorldTickRate,
        "HeadLocation", sol::property(&APlayerCharacter::GetHeadLocation, &APlayerCharacter::SetHeadLocation),
        "HeadRotation", sol::property(&APlayerCharacter::GetHeadRotation, &APlayerCharacter::SetHeadRotation)
    )
}

bool APlayerCharacter::BindSelfLuaProperties()
{
    if (!LuaScriptComponent)
    {
        return false;
    }
    // LuaScript Load 실패.
    if (!LuaScriptComponent->LoadScript())
    {
        return false;
    }

    sol::table& LuaTable = LuaScriptComponent->GetLuaSelfTable();
    if (!LuaTable.valid())
    {
        return false;
    }

    // 자기 자신 등록.
    // self에 this를 하게 되면 내부에서 임의로 Table로 바꿔버리기 때문에 self:함수() 형태의 호출이 불가능.
    // 자기 자신 객체를 따로 넘겨주어야만 AActor:GetName() 같은 함수를 실행시켜줄 수 있다.
    LuaTable["this"] = this;
    LuaTable["Name"] = *GetName(); // FString 해결되기 전까지 임시로 Table로 전달.
    // 이 아래에서 또는 하위 클래스 함수에서 멤버 변수 등록.

    return true;
}

void APlayerCharacter::Punch()
{
    if(UAnimSingleNodeInstance* Instance = LeftArm->GetSingleNodeInstance())
    {
        Instance->SetPlaying(true);
        if (bAnimRestart)
        {
            Instance->SetElapsedTime(0.f);
        }
    }
    AFist* Fist = GetWorld()->SpawnActor<AFist>();
    Fist->SetActorLabel(TEXT("Fist"));
    Fist->SetOwner(this);
    Fist->SetShooter(this);
    Fist->bVisible = false;
    Fist->bSpawned = false;
    Fist->WaitTime = 0.3f;
    Fist->SetShooter(this);
}

void APlayerCharacter::Shoot()
{
    if (UAnimSingleNodeInstance* Instance = RightArm->GetSingleNodeInstance())
    {
        Instance->SetPlaying(true);
        if (bAnimRestart)
        {
            Instance->SetElapsedTime(0.f);
        }
    }
    AFist* Fist = GetWorld()->SpawnActor<AFist>();
    Fist->SetActorLabel(TEXT("Bullet"));
    Fist->SetOwner(this);
    Fist->bVisible = true;
    Fist->bSpawned = false;
    Fist->WaitTime = 1.f;
    Fist->InitialSpeed = 30.f;
    Fist->Lifetime = 5.f;
    Fist->SetShooter(this);
}

void APlayerCharacter::SetPlayRate(float PlayRate)
{
    if (UAnimSingleNodeInstance* Instance = LeftArm->GetSingleNodeInstance())
    {
        Instance->SetPlayRate(PlayRate);
    }
    if (UAnimSingleNodeInstance* Instance = RightArm->GetSingleNodeInstance())
    {
        Instance->SetPlayRate(PlayRate);
    }
}

void APlayerCharacter::SetWorldTickRate(float TickRate)
{
    GEngineLoop.DeltaTimeMultiplier = TickRate;
}

bool APlayerCharacter::IsPunching()
{
    if (UAnimSingleNodeInstance* Instance = LeftArm->GetSingleNodeInstance())
    {
        return Instance->IsPlaying();
    }
    return false;
}

bool APlayerCharacter::IsShooting()
{
    if (UAnimSingleNodeInstance* Instance = RightArm->GetSingleNodeInstance())
    {
        return Instance->IsPlaying();
    }
    return false;
}
