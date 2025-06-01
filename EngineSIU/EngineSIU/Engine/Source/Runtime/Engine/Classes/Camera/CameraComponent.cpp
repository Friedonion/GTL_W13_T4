#include "CameraComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/SpringArmComponent.h"
#include "UObject/Casts.h"
#include "World/World.h"
#include "Actors/Player/Player.h"

UObject* UCameraComponent::Duplicate(UObject* InOuter)
{
    ThisClass* NewComponent = Cast<ThisClass>(Super::Duplicate(InOuter));

    NewComponent->ViewFOV = ViewFOV;
    NewComponent->NearClip = NearClip;
    NewComponent->FarClip = FarClip;
    
    return NewComponent;
}

void UCameraComponent::InitializeComponent()
{
    USceneComponent::InitializeComponent();

    // SetFInterpToSpeed(0.8f);
}

void UCameraComponent::TickComponent(float DeltaTime)
{
    USceneComponent::TickComponent(DeltaTime);
    /*
    if (bFollowCustomTarget) {
        //
    }
    else 
    {
        FollowMainPlayer();
    }
    
    ProceedFInterp(DeltaTime);
    */
}

void UCameraComponent::FollowMainPlayer()
{
    FVector PlayerLocation = GEngine->ActiveWorld->GetMainPlayer()->GetActorLocation();
    
    FVector PlayerBackward = -GEngine->ActiveWorld->GetMainPlayer()->GetActorForwardVector();

    FVector CameraOffset = PlayerBackward * DistanceBehind + FVector(0, 0, CameraHeight);
    
    FVector MoveToLocation = FVector(PlayerLocation.X, PlayerLocation.Y, CameraZ) + CameraOffset;
    
    SetLocationWithFInterpTo(MoveToLocation);

    SetLookTarget(PlayerLocation);
}

void UCameraComponent::ProceedFInterp(float DeltaTime)
{
    /* SpringArmComponent가 부모일 경우 부모 트랜스폼 따라가도록 함 */
    if (USpringArmComponent* SpringArmComp = Cast<USpringArmComponent>(GetAttachParent()))
    {

    }    
    else
    {
        FVector FromLocation = GetComponentLocation();

        //카메라 위치
        FVector MoveLocation = FMath::VInterpTo(FromLocation, FInterpTargetLocation, DeltaTime, FInterpToSpeed);

        FVector Lookat = LookTarget;
    
        CurrentCameraZ = FMath::FInterpTo(CurrentCameraZ, CameraZ, DeltaTime, 0.05f);
        Lookat.Z = CurrentCameraZ + CameraZOffset;
    
        FRotator TargetRotation = FRotator::MakeLookAtRotation(MoveLocation, Lookat);
    
        SetWorldLocation(MoveLocation);
        SetWorldRotation(TargetRotation);
    }
   
   
}

void UCameraComponent::SetLocationWithFInterpTo(const FVector& ToLocation) //LerpSpeed = 0은 안움직이고 1은 바로이동
{
    FInterpTargetLocation = ToLocation;
}

void UCameraComponent::SetFInterpToSpeed(float InSpeed)
{
    FInterpToSpeed = InSpeed;
}

void UCameraComponent::SetLookTarget(FVector& Location)
{
    LookTarget = Location;
}

void UCameraComponent::SetFollowCustomTarget(const FVector& InLocation)
{
    bFollowCustomTarget = true;
    FInterpTargetLocation = InLocation;
}

void UCameraComponent::ResetFollowToPlayer()
{
    bFollowCustomTarget = false;
}


void UCameraComponent::GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView)
{

    DesiredView.Location = GetComponentLocation();
    DesiredView.Rotation = GetComponentRotation();
    DesiredView.FOV = ViewFOV;
    DesiredView.PerspectiveNearClip = NearClip;
    DesiredView.PerspectiveFarClip = FarClip;
    
    // See if the CameraActor wants to override the PostProcess settings used.
    // DesiredView.PostProcessBlendWeight = PostProcessBlendWeight;
    // if (PostProcessBlendWeight > 0.0f)
    // {
    //     DesiredView.PostProcessSettings = PostProcessSettings;
    // }
}
