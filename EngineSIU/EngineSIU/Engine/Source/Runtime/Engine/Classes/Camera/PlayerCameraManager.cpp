#include "PlayerCameraManager.h"

#include "CameraModifier.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraModifier_CameraShake.h"
#include "World/World.h"
#include "Actors/Player/Player.h"
#include "Components/SkeletalMeshComponent.h"

bool FTViewTarget::Equal(const FTViewTarget& OtherTarget) const
{
    //@TODO: Should I compare Controller too?
    return (Target == OtherTarget.Target) && POV.Equals(OtherTarget.POV);
}

void FTViewTarget::CheckViewTarget(APlayerController* OwningController)
{
    if (Target == nullptr)
    {
        Target = OwningController;
    }
    
    if (Target != nullptr)
    {
        // PossessActor가 있을때
        if (OwningController->GetCharacter() && !OwningController->GetCharacter()->IsActorBeingDestroyed() )
        {
            OwningController->PlayerCameraManager->AssignViewTarget(OwningController->GetCharacter(), *this);
        }
        else
        {
            OwningController->PlayerCameraManager->AssignViewTarget(OwningController, *this);
        }
    }
}

void FTViewTarget::SetNewTarget(AActor* NewTarget)
{
    Target = NewTarget;
}

AActor* FTViewTarget::GetTargetActor() const
{
    if (Target)
    {
        return Target;
    }

    if (APlayerController* Controller = Cast<APlayerController>(Target))
    {
        return Controller->GetCharacter();
    }

    return nullptr;
}

APlayerCameraManager::APlayerCameraManager()
{
    DefaultFOV = 90.0f;
    DefaultAspectRatio = 1.33333f;
    bDefaultConstrainAspectRatio = false;

    ViewPitchMin = -89.9f;
    ViewPitchMax = 89.9f;
    ViewYawMin = 0.f;
    ViewYawMax = 359.999f;
    ViewRollMin = -89.9f;
    ViewRollMax = 89.9f;

    F_Stop = 2.8f;
    SensorWidth = 24.576f; // mm
    FocalDistance = 0.f; // cm
}

void APlayerCameraManager::PostSpawnInitialize()
{
    AActor::PostSpawnInitialize();

    CachedCameraShakeMod = new UCameraModifier_CameraShake();
    ModifierList.Add(CachedCameraShakeMod);
}

void APlayerCameraManager::InitializeFor(APlayerController* PC)
{
    PCOwner = PC;
    VignetteColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
    VignetteCenter = FVector2D(0.5f, 0.5f);
    VignetteRadius = 0.5f;
    VignetteSmoothness = 0.5f;
    VignetteIntensity = 0.0f;
    LetterBoxWidth = 2.0f;
    LetterBoxHeight = 1.0f;
}

AActor* APlayerCameraManager::GetViewTarget() const
{
    return ViewTarget.Target;
}

void APlayerCameraManager::UpdateCamera(float DeltaTime)
{
    if (PCOwner)
    {
        DoUpdateCamera(DeltaTime);
    }

}

/**
 * @param FromAlpha          시작 알파값 (0: 완전 투명, 1: 완전 불투명)
 * @param ToAlpha            목표 알파값
 * @param InFadeTime         페이드 지속 시간 (초 단위)
 * @param InFadeColor        페이드에 사용될 색상
 * @param bInHoldWhenFinished 페이드 종료 후의 화면 유지 여부
 */
void APlayerCameraManager::StartCameraFade(float FromAlpha, float ToAlpha, float InFadeTime, FLinearColor InFadeColor, bool bInHoldWhenFinished)
{
    bEnableFading = true;

    FadeColor = InFadeColor;
    FadeAlpha = FVector2D(FromAlpha, ToAlpha);
    FadeTime = InFadeTime;
    FadeTimeRemaining = InFadeTime;

    /* 오디오 효과 중략 */

    bHoldFadeWhenFinished = bInHoldWhenFinished;
}

void APlayerCameraManager::StopCameraFade()
{    
    if (bEnableFading == true)
    {
        // Make sure FadeAmount finishes at the desired value
        FadeAmount = FadeAlpha.Y;
        bEnableFading = false;
        //StopAudioFade();
    }
}

void APlayerCameraManager::ApplyCameraModifiers(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
    TArray<UCameraModifier*> LocalModifierList = ModifierList;

    for (int32 ModifierIdx = 0; ModifierIdx < LocalModifierList.Num(); ++ModifierIdx)
    {
        bool bContinue = true;

        UCameraModifier* CameraModifier = LocalModifierList[ModifierIdx];
        if (CameraModifier)
        {
            bContinue = !CameraModifier->ModifyCamera(DeltaTime, InOutPOV);
        }

        if (!bContinue)
        {
            return;
        }
    }
}

UCameraShakeBase* APlayerCameraManager::StartCameraShake(UClass* ShakeClass)
{
    if (ShakeClass && CachedCameraShakeMod)
    {
        return CachedCameraShakeMod->AddCameraShake(ShakeClass);
    }

    return nullptr;
}

void APlayerCameraManager::StopCameraShake(UCameraShakeBase* ShakeInst, bool bImmediately)
{
    if (ShakeInst && CachedCameraShakeMod)
    {
        CachedCameraShakeMod->RemoveCameraShake(ShakeInst, bImmediately);
    }
}

void APlayerCameraManager::StopAllInstancesOfCameraShake(UClass* ShakeClass, bool bImmediately)
{
    if (ShakeClass && CachedCameraShakeMod)
    {
        CachedCameraShakeMod->RemoveAllCameraShakesOfClass(ShakeClass, bImmediately);
    }
}

void APlayerCameraManager::DoUpdateCamera(float DeltaTime)
{
    //Progress가 진행중이 아니면
    if (PendingViewTarget.Target == nullptr)
    {
        ViewTarget.CheckViewTarget(PCOwner);
        UpdateViewTarget(ViewTarget, DeltaTime);
    }

    FMinimalViewInfo NewPOV = ViewTarget.POV;
    
    if (PendingViewTarget.Target != nullptr)
    {        
        BlendTimeToGo -= DeltaTime;

        // UpdateViewTarget(ViewTarget, DeltaTime);
        // PendingViewTarget.CheckViewTarget(PCOwner);
        UpdateViewTarget(PendingViewTarget, DeltaTime);

        if (BlendTimeToGo > 0)
        {
            float DurationPct = (BlendParams.BlendTime - BlendTimeToGo) / BlendParams.BlendTime;

            float BlendPct = 0.f;
            switch (BlendParams.BlendFunction)
            {
            case VTBlend_Linear:
                BlendPct = FMath::Lerp(0.f, 1.f, DurationPct);
                break;
            case VTBlend_Cubic:
                BlendPct = FMath::CubicInterp(0.f, 0.f, 1.f, 0.f, DurationPct);
                break;
            case VTBlend_EaseIn:
                BlendPct = FMath::Lerp(0.f, 1.f, FMath::Pow(DurationPct, BlendParams.BlendExp));
                break;
            case VTBlend_EaseOut:
                BlendPct = FMath::Lerp(0.f, 1.f, FMath::Pow(DurationPct, 1.f / BlendParams.BlendExp));
                break;
            case VTBlend_EaseInOut:
                BlendPct = FMath::InterpEaseInOut(0.f, 1.f, DurationPct, BlendParams.BlendExp);
                break;
            case VTBlend_PreBlended:
                BlendPct = 1.0f;
                break;
            default:
                break;
            }
            
            // Update pending view target blend
            NewPOV = ViewTarget.POV;
            //@TODO: CAMERA: Make sure the sense is correct!  BlendViewTargets(ViewTarget, PendingViewTarget, BlendPct);
            NewPOV.BlendViewInfo(PendingViewTarget.POV, BlendPct);
            
            UE_LOG(ELogLevel::Error, TEXT("%.2f %.2f %.2f"), NewPOV.Location.X, NewPOV.Location.Y, NewPOV.Location.Z);
        }
        else
        {
            PendingViewTarget.Target = nullptr;

            BlendTimeToGo = 0;

            NewPOV = PendingViewTarget.POV;

            OnBlendCompleteEvent.Broadcast();
        }
    }
    
    // Fade Enabled 되었다면 Fade 처리 수행
    if (bEnableFading)
    {
        FadeTimeRemaining = FMath::Max(FadeTimeRemaining - DeltaTime, 0.0f);
        if (FadeTime > 0.0f)
        {
            FadeAmount = FadeAlpha.X + ((1.f - FadeTimeRemaining / FadeTime) * (FadeAlpha.Y - FadeAlpha.X));
        }

        if (/*(bHoldFadeWhenFinished == false) && */(FadeTimeRemaining <= 0.f))
        {
            // done
            StopCameraFade();
        }
    }

    
    if (bAnimateVignette)
    {
        VignetteTimeRemaining = FMath::Max(VignetteTimeRemaining - DeltaTime, 0.0f);
        if (VignetteTime > 0.0f)
        {
            VignetteIntensity = VignetteStartIntensity + ((1.f - VignetteTimeRemaining / VignetteTime) * (VignetteTargetIntensity - VignetteStartIntensity));

            SetCameraVignette(VignetteIntensity, 0.8f, 0.8f);
        }

        if (VignetteTimeRemaining <= 0.0f)
        {
            VignetteIntensity = VignetteTargetIntensity;
            bAnimateVignette = false;
        }
    }
    
    LastFrameViewTarget.POV = NewPOV;
}

void APlayerCameraManager::SetViewTarget(class AActor* NewTarget, struct FViewTargetTransitionParams TransitionParams)
{
	// Make sure view target is valid
	if (NewTarget == nullptr)
	{
		NewTarget = PCOwner;
	}

    if (PendingViewTarget.Target)
    {
        return;
    }
    
	if (TransitionParams.BlendTime > 0)
	{

		BlendTimeToGo = TransitionParams.BlendTime;

		AssignViewTarget(PCOwner->GetCharacter(), ViewTarget);
		AssignViewTarget(NewTarget, PendingViewTarget, TransitionParams);

	}

	BlendParams = TransitionParams;
}

void APlayerCameraManager::UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime)
{
    // Don't update outgoing viewtarget during an interpolation 
    if (PendingViewTarget.Target != nullptr && OutVT.Equal(ViewTarget))
	{
		return;
	}

	// Store previous POV, in case we need it later
	FMinimalViewInfo OrigPOV = OutVT.POV;

	// Reset the view target POV fully
	static const FMinimalViewInfo DefaultViewInfo;
	OutVT.POV = DefaultViewInfo;
	OutVT.POV.FOV = DefaultFOV;

	bool bDoNotApplyModifiers = false;

    OutVT.POV.Location = OutVT.Target->GetActorLocation();
    OutVT.POV.Rotation = OutVT.Target->GetActorRotation();
    if (APlayerCharacter* Player = Cast<APlayerCharacter>(OutVT.Target))
    {
		OutVT.POV.Location = Player->GetHeadLocation();
        OutVT.POV.Location += FVector(0.f, 0.f, Player->BaseEyeHeight);
		OutVT.POV.Rotation = Player->GetHeadRotation();
    }
    OutVT.POV.Rotation.Roll = 0.0f;
    
	if (UCameraComponent* CamComp = OutVT.Target->GetComponentByClass<UCameraComponent>())
	{
		// Viewing through a camera actor.
		CamComp->GetCameraView(DeltaTime, OutVT.POV);
	}
    else 
    {
        for (auto It : TObjectRange<UCameraComponent>())
        {
            It->GetCameraView(DeltaTime, OutVT.POV);
        }
    }
    
	ApplyCameraModifiers(DeltaTime, OutVT.POV);
}

void APlayerCameraManager::SetCameraVignette(float InIntensity, float InRadius, float InSmoothness)
{
    VignetteIntensity = InIntensity;
    VignetteRadius = InRadius;
    VignetteSmoothness = InSmoothness;
}

void APlayerCameraManager::SetCameraVignetteColor(FLinearColor InColor)
{
    VignetteColor = InColor;
}

/* A로부터 B로의 ViewTarget Blend 수행
 * 실제 언리얼 코드에선 사용하지 않음
 */
FMinimalViewInfo APlayerCameraManager::BlendViewTargets(const FTViewTarget& A, const FTViewTarget& B, float Alpha)
{
    FMinimalViewInfo POV;
    POV.Location = FMath::Lerp(A.POV.Location, B.POV.Location, Alpha);
    POV.FOV = (A.POV.FOV + Alpha * (B.POV.FOV - A.POV.FOV));

    FRotator DeltaAng = (B.POV.Rotation - A.POV.Rotation).GetNormalized();
    POV.Rotation = A.POV.Rotation + DeltaAng * Alpha;

    return POV;
}

float APlayerCameraManager::GetFocalLength() const
{
    const float FovRad = FMath::DegreesToRadians(DefaultFOV);
    const float FocalLength = SensorWidth / (2.f * FMath::Tan(FovRad / 2.0f));
    return FocalLength;
}

void APlayerCameraManager::AssignViewTarget(AActor* NewTarget, FTViewTarget& VT, struct FViewTargetTransitionParams TransitionParams)
{
    if (!NewTarget || NewTarget == VT.Target)
    {
        return;
    }
    
    // AActor* OldViewTarget = VT.Target;
    VT.Target = NewTarget;

    // Use default FOV and aspect ratio.
    VT.POV.FOV = DefaultFOV;

}

void APlayerCameraManager::StartVignetteAnimation(float FromIntensity, float ToIntensity, float Duration)
{
    bAnimateVignette = true;
    VignetteStartIntensity = FromIntensity;
    VignetteTargetIntensity = ToIntensity;
    VignetteTime = Duration;
    VignetteTimeRemaining = Duration;
}

float APlayerCameraManager::GetLetterBoxRatio()
{
    return LetterBoxWidth / LetterBoxHeight;
}
