#pragma once
#include "ParticleModuleVelocityBase.h"
#include "Classes/Distribution/DistributionVector.h"
#include "Classes/Distribution/DistributionFloat.h"

struct FDistributionVector;

class UParticleModuleVelocity : public UParticleModuleVelocityBase
{
    DECLARE_CLASS(UParticleModuleVelocity, UParticleModuleVelocityBase)

public:
    UParticleModuleVelocity();
    virtual ~UParticleModuleVelocity() override = default;

    virtual void DisplayProperty() override;

    virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    //virtual void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;

    UPROPERTY_WITH_FLAGS(EditAnywhere, FDistributionVector, StartVelocity)
    UPROPERTY_WITH_FLAGS(EditAnywhere, FDistributionFloat, StartVelocityRadial)

    virtual void SerializeAsset(FArchive& Ar) override;
};
