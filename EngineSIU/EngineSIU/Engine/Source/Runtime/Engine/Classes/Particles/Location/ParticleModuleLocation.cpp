#include "ParticleModuleLocation.h"
#include "Engine/ParticleEmitterInstance.h"
#include "Particles/ParticleSystemComponent.h"

UParticleModuleLocation::UParticleModuleLocation()
{
    bSpawnModule = true;
    bUpdateModule = false;
    bInWorldSpace = false;
    bApplyEmitterLocation = true;

    StartLocation.MinValue = FVector::OneVector * -3;
    StartLocation.MaxValue = FVector::OneVector * 3;

    LocationOffset.MinValue = FVector::ZeroVector;
    LocationOffset.MaxValue = FVector::ZeroVector;

    ModuleName = "Location";
}

void UParticleModuleLocation::DisplayProperty()
{
    Super::DisplayProperty();
    for (const auto& Property : StaticClass()->GetProperties())
    {
        ImGui::PushID(Property);
        Property->DisplayInImGui(this);
        ImGui::PopID();
    }
}

void UParticleModuleLocation::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
    FVector Location = StartLocation.GetValue();
    FVector OffsetLocation = LocationOffset.GetValue();

    Location += OffsetLocation;

    //if (bInWorldSpace)
    //{
    //    Location = Owner->Component->GetComponentToWorld().TransformPosition(Location);
    //}

    //if (bApplyEmitterLocation)
    //{
    //    Location += Owner->Component->GetComponentToWorld().GetTranslation();
    //}

    Location = Owner->Component->GetComponentTransform().TransformPosition(Location);

    ParticleBase->Location = Location;
}

void UParticleModuleLocation::SerializeAsset(FArchive& Ar)
{
    Super::SerializeAsset(Ar);

    Ar << StartLocation << LocationOffset;
}
