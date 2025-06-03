#include "ParticleModuleVelocityOverLife.h"
#include "Engine/ParticleHelper.h"
#include "Engine/ParticleEmitterInstance.h"
#include "Particles/ParticleSystemComponent.h"

UParticleModuleVelocityOverLife::UParticleModuleVelocityOverLife()
{
    bSpawnModule = false;
    bUpdateModule = true;

    bInWorldSpace = false;
    bApplyOwnerScale = false;

    bUseConstantChange = true;
    bUseVelocityCurve = false;

    CurveScale = 10.0f;

    StartVelocity = FVector::ZeroVector;
    EndVelocity = FVector::ZeroVector;

    ModuleName = "VelocityOverLife";
}

void UParticleModuleVelocityOverLife::DisplayProperty()
{
    Super::DisplayProperty();

    // 상호 배타적 옵션 동기화
    if (bUseConstantChange && bUseVelocityCurve)
    {
        if (ImGui::IsItemEdited()) bUseVelocityCurve = false;
    }
    else if (bUseVelocityCurve && bUseConstantChange)
    {
        if (ImGui::IsItemEdited()) bUseConstantChange = false;
    }

    for (const auto& Property : StaticClass()->GetProperties())
    {
        ImGui::PushID(Property);
        Property->DisplayInImGui(this);
        ImGui::PopID();
    }
}

void UParticleModuleVelocityOverLife::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime)
{
    BEGIN_UPDATE_LOOP

        // payload 위치 계산
        uint8* writableBase = const_cast<uint8*>(ParticleBase);
    FVector* InitialVelPtr = reinterpret_cast<FVector*>(writableBase + ModulePayloadOffset);

    float RelTime = Particle.RelativeTime;
    float firstFrameTime = DeltaTime * Particle.OneOverMaxLifetime;

    FVector InitialVelocity;
    // 첫번쨰 프레임일 때는 값을 쓰고
    if (RelTime < firstFrameTime)
    {
        InitialVelocity = Particle.BaseVelocity;
        *InitialVelPtr = InitialVelocity;
    }

    InitialVelocity = *InitialVelPtr;

    FVector NewVelocity = FVector::ZeroVector;
    if (bUseConstantChange)
    {
        float Alpha = FMath::Clamp(RelTime, 0.0f, 1.0f);
        NewVelocity = FMath::Lerp(InitialVelocity, EndVelocity, Alpha);
    }
    else if (bUseVelocityCurve)
    {
        //FVector CurrentVelocity = Particle.BaseVelocity;
        //float Alpha = FMath::Lerp(0.0f, 1.0f, RelTime);

        float t = FMath::Clamp(Particle.RelativeTime, 0.0f, 1.0f);
        float curveAlpha = 1.0f - FMath::Pow(1.0f - t, CurveScale);
        NewVelocity = FMath::Lerp(InitialVelocity, EndVelocity, curveAlpha);
    }

    // 월드공간 변환 및 오너 스케일 적용
    //if (bInWorldSpace)
    //{
    //    NewVelocity = Owner->Component->GetComponentToWorld().TransformVector(NewVelocity);
    //}
    //if (bApplyOwnerScale)
    //{
    //    NewVelocity *= Owner->Component->GetComponentToWorld().GetScale3D();
    //}

    Particle.BaseVelocity = NewVelocity;

    END_UPDATE_LOOP
}

void UParticleModuleVelocityOverLife::SerializeAsset(FArchive& Ar)
{
    Super::SerializeAsset(Ar);

    Ar << bUseConstantChange << bUseVelocityCurve << CurveScale << StartVelocity << EndVelocity;
}
