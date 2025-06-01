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

    Super::BeginPlay();
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
    DEFINE_LUA_TYPE_WITH_PARENT(APlayerCharacter, ACharacter,
        "State", sol::property(&ThisClass::State))
}

//bool APlayerCharacter::BindSelfLuaProperties()
//{
//    if (!LuaScriptComponent)
//    {
//        return false;
//    }
//    // LuaScript Load 실패.
//    if (!LuaScriptComponent->LoadScript())
//    {
//        return false;
//    }
//
//    sol::table& LuaTable = LuaScriptComponent->GetLuaSelfTable();
//    if (!LuaTable.valid())
//    {
//        return false;
//    }
//
//    // 자기 자신 등록.
//    // self에 this를 하게 되면 내부에서 임의로 Table로 바꿔버리기 때문에 self:함수() 형태의 호출이 불가능.
//    // 자기 자신 객체를 따로 넘겨주어야만 AActor:GetName() 같은 함수를 실행시켜줄 수 있다.
//    LuaTable["this"] = this;
//    LuaTable["Name"] = *GetName(); // FString 해결되기 전까지 임시로 Table로 전달.
//    // 이 아래에서 또는 하위 클래스 함수에서 멤버 변수 등록.
//
//    return true;
//}
