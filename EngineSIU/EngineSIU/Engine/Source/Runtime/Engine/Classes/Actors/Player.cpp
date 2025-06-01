#include "Player.h"
#include "Engine/Engine.h"
#include "World/World.h"
#include "Engine/SkeletalMesh.h"
#include "Lua/LuaScriptComponent.h"
#include "Lua/LuaScriptManager.h"
#include "Lua/LuaUtils/LuaTypeMacros.h"
#include "Classes/Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"

APlayerCharacter::APlayerCharacter()
    : ACharacter()
{
    LeftArm = AddComponent<USkeletalMeshComponent>(FName("LeftArm"));
    RightArm = AddComponent<USkeletalMeshComponent>(FName("RightArm"));
    
    LeftArm->SetupAttachment(Super::Mesh);
    RightArm->SetupAttachment(Super::Mesh);
}

void APlayerCharacter::BeginPlay()
{
    Super::BeginPlay();

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
    // C++코드를 호출
    LuaScriptComponent->ActivateFunction("InitializeCallback");
}

UObject* APlayerCharacter::Duplicate(UObject* InOuter)
{
    ThisClass* NewActor = Cast<ThisClass>(Super::Duplicate(InOuter));

    return NewActor;
}

void APlayerCharacter::Tick(float DeltaTime)
{
    AActor::Tick(DeltaTime);
}

void APlayerCharacter::RegisterLuaType(sol::state& Lua)
{
    Super::RegisterLuaType(Lua);
}

