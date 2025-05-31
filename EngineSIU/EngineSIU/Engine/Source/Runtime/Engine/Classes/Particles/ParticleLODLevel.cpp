#include "ParticleLODLevel.h"

#include "UObject/Casts.h"
#include "Spawn/ParticleModuleSpawn.h"
#include "ParticleModuleRequired.h"
#include "Color/ParticleModuleColorBase.h"
#include "Color/ParticleModuleColorOverLife.h"
#include "Container/ArrayHelper.h"
#include "Size/ParticleModuleSize.h"
#include "UObject/ObjectFactory.h"
#include "Location/ParticleModuleLocation.h"
#include "Modules/ParticleModuleLifeTime.h"
#include "SubUV/ParticleModuleSubUV.h"
#include "Velocity/ParticleModuleVelocity.h"
#include "Velocity/ParticleModuleVelocityOverLife.h"

UParticleLODLevel::UParticleLODLevel()
{
    RequiredModule = FObjectFactory::ConstructObject<UParticleModuleRequired>(this);
    Modules.Add(RequiredModule);

    SpawnModule = FObjectFactory::ConstructObject<UParticleModuleSpawn>(this);
    Modules.Add(SpawnModule);

    UParticleModuleSize* InitialScaleModule = FObjectFactory::ConstructObject<UParticleModuleSize>(this);
    Modules.Add(InitialScaleModule);

    UParticleModuleLocation* LocationModule = FObjectFactory::ConstructObject<UParticleModuleLocation>(this);
    Modules.Add(LocationModule);

    UParticleModuleVelocity* VelocityModule = FObjectFactory::ConstructObject<UParticleModuleVelocity>(this);
    Modules.Add(VelocityModule);

    UParticleModuleLifeTime* LifeTimeModule = FObjectFactory::ConstructObject<UParticleModuleLifeTime>(this);
    Modules.Add(LifeTimeModule);
    
    UParticleModuleColorBase* ColorModule = FObjectFactory::ConstructObject<UParticleModuleColorBase>(this);
    Modules.Add(ColorModule);

    UParticleModuleColorOverLife* ColorOverLifeModule = FObjectFactory::ConstructObject<UParticleModuleColorOverLife>(this);
    Modules.Add(ColorOverLifeModule);

    UParticleModuleVelocityOverLife* VelocityOverLifeModule = FObjectFactory::ConstructObject<UParticleModuleVelocityOverLife>(this);
    Modules.Add(VelocityOverLifeModule);

    UParticleModuleSubUV* SubUVModule = FObjectFactory::ConstructObject<UParticleModuleSubUV>(this);
    Modules.Add(SubUVModule);
}

int32 UParticleLODLevel::CalculateMaxActiveParticleCount()
{
    // Determine the lifetime for particles coming from the emitter
	float ParticleLifetime = 0.0f;
    float MinSpawnRate;
	float MaxSpawnRate;
    SpawnModule->GetSpawnRate(MinSpawnRate, MaxSpawnRate);
	// int32 MaxBurstCount = SpawnModule->GetMaximumBurstCount();
	for (int32 ModuleIndex = 0; ModuleIndex < Modules.Num(); ModuleIndex++)
	{
		// UParticleModuleLifetimeBase* LifetimeMod = Cast<UParticleModuleLifetimeBase>(Modules[ModuleIndex]);
		// if (LifetimeMod != NULL)
		// {
		// 	ParticleLifetime += LifetimeMod->GetMaxLifetime();
		// }

		UParticleModuleSpawnBase* SpawnMod = Cast<UParticleModuleSpawnBase>(Modules[ModuleIndex]);
		if (SpawnMod != NULL)
		{
			// MaxSpawnRate += SpawnMod->GetMinimumSpawnRate();
		    MaxSpawnRate += MinSpawnRate;
			// MaxBurstCount += SpawnMod->GetMaximumBurstCount();
		}
	}

	// Determine the maximum duration for this particle system
	float MaxDuration = 0.0f;
	float TotalDuration = 0.0f;
	int32 TotalLoops = 0;
	if (RequiredModule != NULL)
	{
		// We don't care about delay wrt spawning...
		// MaxDuration = FMath::Max<float>(RequiredModule->EmitterDuration, RequiredModule->EmitterDurationLow); // TODO: 주석 풀기
		// TotalLoops = RequiredModule->EmitterLoops;
		TotalDuration = MaxDuration * static_cast<float>(TotalLoops);
	}

	// Determine the max
	int32 MaxAPC = 0;

	if (TotalDuration != 0.0f)
	{
		if (TotalLoops == 1)
		{
			// Special case for one loop... 
			if (ParticleLifetime < MaxDuration)
			{
				MaxAPC += FMath::CeilToInt(ParticleLifetime * MaxSpawnRate);
			}
			else
			{
				MaxAPC += FMath::CeilToInt(MaxDuration * MaxSpawnRate);
			}
			// Safety zone...
			MaxAPC += 1;
			// Add in the bursts...
			// MaxAPC += MaxBurstCount;
		}
		else
		{
			if (ParticleLifetime < MaxDuration)
			{
				MaxAPC += FMath::CeilToInt(ParticleLifetime * MaxSpawnRate);
			}
			else
			{
				MaxAPC += (FMath::CeilToInt(FMath::CeilToInt(MaxDuration * MaxSpawnRate) * ParticleLifetime));
			}
			// Safety zone...
			MaxAPC += 1;
			// Add in the bursts...
			// MaxAPC += MaxBurstCount;
			if (ParticleLifetime > MaxDuration)
			{
				// MaxAPC += MaxBurstCount * FMath::CeilToInt(ParticleLifetime - MaxDuration);
			}
		}
	}
	else
	{
		// We are infinite looping... 
		// Single loop case is all we will worry about. Safer base estimate - but not ideal.
		if (ParticleLifetime < MaxDuration)
		{
			MaxAPC += FMath::CeilToInt(ParticleLifetime * FMath::CeilToInt(MaxSpawnRate));
		}
		else
		{
			if (ParticleLifetime != 0.0f)
			{
				if (ParticleLifetime <= MaxDuration)
				{
					MaxAPC += FMath::CeilToInt(MaxDuration * MaxSpawnRate);
				}
				else //if (ParticleLifetime > MaxDuration)
				{
					MaxAPC += FMath::CeilToInt(MaxDuration * MaxSpawnRate * ParticleLifetime);
				}
			}
			else
			{
				// No lifetime, no duration...
				MaxAPC += FMath::CeilToInt(MaxSpawnRate);
			}
		}
		// Safety zone...
		MaxAPC += FMath::Max<int32>(FMath::CeilToInt(MaxSpawnRate * 0.032f), 2);
		// Burst
		// MaxAPC += MaxBurstCount;
	}

	PeakActiveParticles = MaxAPC;

	return MaxAPC;
}

void UParticleLODLevel::SerializeAsset(FArchive& Ar)
{
    Ar << Level << bEnabled << PeakActiveParticles;

    /**
     * Loading인 경우 주의 사항:
     * 
     *   UParticleLODLevel 클래스의 생성자에서 기본 모듈 및 일부 모듈들을 미리 생성해두고 있음.
     *   기본 모듈이 아닌 것들은 언제든지 삭제가 가능하고, 어떤 것들이 삭제되었는지는 현재 시점에서 알 수 없는 문제가 있음.
     *   
     *   따라서 Loading일 때는 생성자에서 생성된 모든 모듈을 삭제하고 저장된 정보에 따라 다시 생성해야 함.
     *   좋은 방법은 생성자에서의 모듈 생성을 막는 것.
     *
     *   SerializePtrAsset 함수에서는 Loading일 때 배열에 원소가 있으면 모두 삭제하고 다시 생성하므로,
     *   현재의 목적과 맞음.
     */
    
    FArrayHelper::SerializePtrAsset(Ar, Modules, this);

    if (Ar.IsLoading())
    {
        bool bHasRequiredModule = false;
        bool bHasSpawnModule = false;
        
        for (const UParticleModule* Module : Modules)
        {
            if (Module->IsA<UParticleModuleRequired>())
            {
                RequiredModule = Cast<UParticleModuleRequired>(Module);
                bHasRequiredModule = true;
                continue;
            }
            if (Module->IsA<UParticleModuleSpawn>())
            {
                SpawnModule = Cast<UParticleModuleSpawn>(Module);
                bHasSpawnModule = true;
                continue;
            }

            if (bHasRequiredModule && bHasSpawnModule)
            {
                break;
            }
        }
    }
}
