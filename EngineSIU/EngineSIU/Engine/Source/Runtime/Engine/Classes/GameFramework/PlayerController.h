#pragma once
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Actor.h" 
#include "Classes/Components/InputComponent.h"

class APlayerCameraManager;

class APlayerController : public AActor
{
    DECLARE_CLASS(APlayerController, AActor)
    
public:
    APlayerController() = default;
    virtual ~APlayerController() override = default;

    virtual void PostSpawnInitialize() override;

    virtual void BeginPlay() override;

    virtual void Tick(float DeltaTime) override;
    
    void ProcessInput(float DeltaTime) const;

    virtual void Destroyed() override;

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UInputComponent* GetInputComponent() const { return InputComponent; }

    void SetViewTarget(class AActor* NewViewTarget, struct FViewTargetTransitionParams TransitionParams);

    virtual void Possess(AActor* InActor);

    virtual void UnPossess();
    
    template<typename Func>
    void BindAction(const FString& Key, Func&& Callback);

    template<typename Func>
    void BindKey(const EKeys::Type Key, Func&& Callback);

    template<typename Func>
    void BindMouseMove(Func&& Callback);

    //template<typename Func>
    //void BindMouseDown(EMouseButtons::Type Button, Func&& Callback);

    AActor* GetPossessedActor() const { return PossessedActor; }
    
    // 카메라 관련 함수
    AActor* GetViewTarget() const;

    virtual void SpawnPlayerCameraManager();

    void ClientStartCameraShake(UClass* Shake);

    void ClientStopCameraShake(UClass* Shake, bool bImmediately = true);

    APlayerCameraManager* PlayerCameraManager = nullptr;
    
protected:
    UPROPERTY(UInputComponent*, InputComponent, = nullptr)

    virtual void SetupInputComponent();

    AActor* PossessedActor = nullptr;

    bool bHasPossessed = false;
};

// 키입력(키보드/마우스)에 대한 콜백 함수를 바인딩합니다.
template<typename Func>
inline void APlayerController::BindAction(const FString& Key, Func&& Callback)
{
    if (InputComponent)
    {
        EKeys::Type KeyType = EKeys::FromString(Key);
        if (EKeys::IsKeyboardKey(KeyType))
        {
            InputComponent->BindKey(KeyType, Callback);
        }
        else if (EMouseButtons::Type ButtonType =  EKeys::ToMouseButton(KeyType))
        {
            // 마우스 버튼에 대한 콜백 바인딩
            InputComponent->BindMouseDown(ButtonType, Callback);
        }
        else
        {
            UE_LOG(ELogLevel::Error, TEXT("Unsupported key type for binding: %s"), *Key);
        }
    }
}

template<typename Func>
inline void APlayerController::BindKey(const EKeys::Type Key, Func&& Callback)
{
    if (InputComponent)
    {
        InputComponent->BindKey(Key, Callback);
    }
}

template<typename Func>
inline void APlayerController::BindMouseMove(Func&& Callback)
{
    if (InputComponent)
    {
        InputComponent->BindMouseMove(Callback);
    }
}
//
//template<typename Func>
//inline void APlayerController::BindMouseDown(EMouseButtons::Type Button, Func&& Callback)
//{
//    if (InputComponent)
//    {
//        InputComponent->BindMouseDown(Button, Callback);
//    }
//}
