#include "PlayerController.h"

#include "Camera/PlayerCameraManager.h"
#include "Engine/Engine.h"
#include "UObject/UObjectIterator.h"
#include "World/World.h"


void APlayerController::PostSpawnInitialize()
{
    AActor::PostSpawnInitialize();

    SetupInputComponent();
    SpawnPlayerCameraManager();
}

void APlayerController::BeginPlay()
{
}

void APlayerController::Tick(float DeltaTime)
{
    AActor::Tick(DeltaTime);
    
    if (bHasPossessed)
    {
        ProcessInput(DeltaTime);
    }

    if (PlayerCameraManager)
    {
        PlayerCameraManager->UpdateCamera(DeltaTime);
    }

}

void APlayerController::ProcessInput(float DeltaTime) const
{
    if (InputComponent)
    {
        InputComponent->ProcessInput(DeltaTime);
    }
}

void APlayerController::Destroyed()
{
    UnPossess();
}

void APlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UnPossess();
}

void APlayerController::SetViewTarget(class AActor* NewViewTarget, struct FViewTargetTransitionParams TransitionParams)
{
    if (PlayerCameraManager)
    {
        PlayerCameraManager->SetViewTarget(NewViewTarget, TransitionParams);
    }
}

void APlayerController::Possess(AActor* InActor)
{
    PossessedActor = InActor;
    bHasPossessed = true;

    if (InputComponent)
    {
        InputComponent->SetPossess();
    }
}

void APlayerController::UnPossess()
{
    if (!bHasPossessed && PossessedActor == nullptr)
    {
        return;
    }
    
    PossessedActor = nullptr;
    bHasPossessed = false;

    if (InputComponent)
    {
        InputComponent->UnPossess();
    }
}

void APlayerController::SetupInputComponent()
{
    // What is the correct parameter of ConstructObject?
    if (InputComponent == nullptr)
    {
        InputComponent = AddComponent<UInputComponent>();
    }
}

AActor* APlayerController::GetViewTarget() const
{
    AActor* CameraManagerViewTarget = PlayerCameraManager ? PlayerCameraManager->GetViewTarget() : nullptr;

    return CameraManagerViewTarget ? CameraManagerViewTarget : const_cast<APlayerController*>(this);
}

void APlayerController::SpawnPlayerCameraManager()
{
    PlayerCameraManager = GetWorld()->SpawnActor<APlayerCameraManager>();

    if (PlayerCameraManager)
    {
        PlayerCameraManager->InitializeFor(this);
    }
}

void APlayerController::ClientStartCameraShake(UClass* Shake)
{
    if (PlayerCameraManager != nullptr)
    {
        PlayerCameraManager->StartCameraShake(Shake);
    }
}

void APlayerController::ClientStopCameraShake(UClass* Shake, bool bImmediately)
{
    if (PlayerCameraManager != nullptr)
    {
        PlayerCameraManager->StopAllInstancesOfCameraShake(Shake, bImmediately);
    }
}
