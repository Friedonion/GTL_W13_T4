#include "ParticleSystemComponent.h"
#include "ParticleEmitterInstance.h"
#include "LevelEditor/SLevelEditor.h"
#include "Particles/ParticleSystem.h"
#include "UnrealEd/EditorViewportClient.h"
#include "ParticleEmitter.h"
#include "Engine/AssetManager.h"
#include "UObject/Casts.h"

UParticleSystemComponent::UParticleSystemComponent()
    : AccumTickTime(0.f)
    , Template(nullptr)
{
}

UObject* UParticleSystemComponent::Duplicate(UObject* InOuter)
{
    ThisClass* NewComponent = Cast<ThisClass>(Super::Duplicate(InOuter));
    NewComponent->SetRelativeTransform(GetRelativeTransform());

    NewComponent->SetParticleSystem(GetParticleSystem());
    
    return NewComponent;
}

void UParticleSystemComponent::InitializeComponent()
{
    Super::InitializeComponent();

    if (Template)
    {
        InitializeSystem();
    }
}

void UParticleSystemComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);

    if (!Template)
    {
        return;
    }

    if (EmitterInstances.Num() != Template->GetEmitters().Num())
    {
        EmitterInstances.Empty();
        InitializeSystem();
    }

    for (auto* Instance : EmitterInstances)
    {
        if (Instance)
        {
            Instance->Tick(DeltaTime);
        }
    }

    UpdateDynamicData();
}

void UParticleSystemComponent::GetProperties(TMap<FString, FString>& OutProperties) const
{
    Super::GetProperties(OutProperties);

    const FName TemplateKey = UAssetManager::Get().GetAssetKeyByObject(EAssetType::ParticleSystem, Template);
    OutProperties.Add(TEXT("TemplateKey"), TemplateKey.ToString());
}

void UParticleSystemComponent::SetProperties(const TMap<FString, FString>& InProperties)
{
    Super::SetProperties(InProperties);

    if (InProperties.Contains(TEXT("TemplateKey")))
    {
        FName TemplateKey = InProperties[TEXT("TemplateKey")];
        UObject* Object = UAssetManager::Get().GetAsset(EAssetType::ParticleSystem, TemplateKey);
        if (UParticleSystem* ParticleSystem = Cast<UParticleSystem>(Object))
        {
            SetParticleSystem(ParticleSystem);
        }
    }
}

void UParticleSystemComponent::InitializeSystem()
{
    TArray<UParticleEmitter*> Emitters = Template->GetEmitters();
    for (int32 i = 0; i < Emitters.Num(); i++)
    {
        UParticleEmitter* EmitterTemplate = Emitters[i];
        if (EmitterTemplate->EmitterType == EDynamicEmitterType::DET_Sprite)
        {
            CreateAndAddSpriteEmitterInstance(EmitterTemplate);
        }
        else if (EmitterTemplate->EmitterType == EDynamicEmitterType::DET_Mesh)
        {
            CreateAndAddMeshEmitterInstance(EmitterTemplate);
        }
    }
}

void UParticleSystemComponent::CreateAndAddSpriteEmitterInstance(UParticleEmitter* EmitterTemplate)
{
    if (EmitterTemplate)
    {
        FParticleSpriteEmitterInstance* Instance = new FParticleSpriteEmitterInstance();
        Instance->SpriteTemplate = EmitterTemplate;
        Instance->Component = this;
        Instance->CurrentLODLevelIndex = 0;

        Instance->Initialize();

        EmitterInstances.Add(Instance);
    }
}

void UParticleSystemComponent::CreateAndAddMeshEmitterInstance(UParticleEmitter* EmitterTemplate)
{
    if (EmitterTemplate)
    {
        FParticleMeshEmitterInstance* Instance = new FParticleMeshEmitterInstance();
        Instance->SpriteTemplate = EmitterTemplate;
        Instance->Component = this;
        Instance->CurrentLODLevelIndex = 0;

        Instance->Initialize();

        EmitterInstances.Add(Instance);
    }
}

void UParticleSystemComponent::UpdateDynamicData()
{
    // Create the dynamic data for rendering this particle system
    if (ParticleDynamicData)
    {
        delete ParticleDynamicData;
    }
    ParticleDynamicData = CreateDynamicData();
}

FParticleDynamicData* UParticleSystemComponent::CreateDynamicData()
{
    if (EmitterInstances.Num() > 0)
    {
        int32 LiveCount = 0;
        for (int32 EmitterIndex = 0; EmitterIndex < EmitterInstances.Num(); EmitterIndex++)
        {
            FParticleEmitterInstance* EmitInst = EmitterInstances[EmitterIndex];
            if (EmitInst)
            {
                if (EmitInst->ActiveParticles > 0)
                {
                    LiveCount++;
                }
            }
        }

        if (LiveCount == 0)
        {
            return nullptr;
        }
    }

    FParticleDynamicData* ParticleDynamicData = new FParticleDynamicData();

    if (Template)
    {
        ParticleDynamicData->SystemPositionForMacroUVs = GetComponentTransform().TransformPosition(Template->GetMacroUVPosition());
        ParticleDynamicData->SystemRadiusForMacroUVs = Template->GetMacroUVRadius();
    }

    ParticleDynamicData->DynamicEmitterDataArray.Empty();
    ParticleDynamicData->DynamicEmitterDataArray.Reserve(EmitterInstances.Num());

    for (int32 EmitterIndex = 0; EmitterIndex < EmitterInstances.Num(); EmitterIndex++)
    {
        FDynamicEmitterDataBase* NewDynamicEmitterData = nullptr;
        FParticleEmitterInstance* EmitterInst = EmitterInstances[EmitterIndex];

        if (EmitterInst)
        {
            NewDynamicEmitterData = EmitterInst->GetDynamicData(true);

            if (NewDynamicEmitterData != nullptr)
            {
                ParticleDynamicData->DynamicEmitterDataArray.Add(NewDynamicEmitterData);
                NewDynamicEmitterData->EmitterIndex = EmitterIndex;
            }
        }
    }

    return ParticleDynamicData;
}

void UParticleSystemComponent::ReBuildInstancesMemoryLayout()
{
    for (auto* Instance : EmitterInstances)
    {
        if (Instance)
        {
            Instance->AllKillParticles();
            Instance->BuildMemoryLayout();
        }
    }
}
