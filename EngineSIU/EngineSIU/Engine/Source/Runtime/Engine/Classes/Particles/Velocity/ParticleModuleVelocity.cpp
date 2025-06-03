#include "ParticleModuleVelocity.h"
#include "ParticleHelper.h"
#include "Engine/ParticleEmitterInstance.h"
#include "Particles/ParticleSystemComponent.h"

UParticleModuleVelocity::UParticleModuleVelocity()
{
    bSpawnModule = true;
    bUpdateModule = false;

    bInWorldSpace = false;
    bApplyOwnerScale = false;

    ModuleName = "Velocity";
}

void UParticleModuleVelocity::DisplayProperty()
{
    Super::DisplayProperty();
    for (const auto& Property : StaticClass()->GetProperties())
    {
        ImGui::PushID(Property);
        Property->DisplayInImGui(this);
        ImGui::PopID();
    }

}

void UParticleModuleVelocity::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
    FVector Velocity = StartVelocity.GetValue();
    float RadialStrength = StartVelocityRadial.GetValue();

    if (!FMath::IsNearlyZero(RadialStrength))
    {
        FVector EmitterLocation = Owner->Component->GetComponentTransform().GetTranslation();
        FVector Direction = (ParticleBase->Location - EmitterLocation).GetSafeNormal();
        Velocity += Direction * RadialStrength;
    }

    //if (bInWorldSpace)
    //{
    //    Velocity = Owner->Component->GetComponentToWorld().TransformVector(Velocity);
    //}

    //if (bApplyOwnerScale)
    //{
    //    Velocity *= Owner->Component->GetComponentToWorld().GetScale3D();
    //}

    ParticleBase->BaseVelocity = Velocity;
}

void UParticleModuleVelocity::SerializeAsset(FArchive& Ar)
{
    Super::SerializeAsset(Ar);

    Ar << StartVelocity << StartVelocityRadial;
}
