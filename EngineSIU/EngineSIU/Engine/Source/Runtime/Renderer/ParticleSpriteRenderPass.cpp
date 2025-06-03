#include "ParticleSpriteRenderPass.h"

#include "RendererHelpers.h"
#include "UnrealClient.h"
#include "Components/Material/Material.h"
#include "D3D11RHI/DXDShaderManager.h"
#include "D3D11RHI/GraphicDevice.h"
#include "Engine/Engine.h"
#include "LevelEditor/SLevelEditor.h"
#include "UnrealEd/EditorViewportClient.h"
#include "UObject/UObjectIterator.h"
#include "Particles/ParticleSystemComponent.h"
#include "ParticleHelper.h"

void FParticleSpriteRenderPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManage)
{
    FRenderPassBase::Initialize(InBufferManager, InGraphics, InShaderManage);

    HRESULT hr = ShaderManager->AddVertexShader(L"ParticleSpriteVertexShader", L"Shaders/ParticleSpriteVertexShader.hlsl", "main");
    if (FAILED(hr))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create ParticleSpriteVertexShader shader!"));
    }

    hr = ShaderManager->AddPixelShader(L"ParticleSpritePixelShader", L"Shaders/ParticleSpritePixelShader.hlsl", "main");
    if (FAILED(hr))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create ParticleSpritePixelShader shader!"));
    }
}

void FParticleSpriteRenderPass::PrepareRenderArr()
{
    for (auto Iter : TObjectRange<UParticleSystemComponent>())
    {
        if (Iter->GetWorld() == GEngine->ActiveWorld)
        {
            FParticleDynamicData* Particle = Iter->GetParticleDynamicData();
            if (Particle && !Particle->DynamicEmitterDataArray.IsEmpty())
            {
                for (auto Emitter : Particle->DynamicEmitterDataArray)
                {
                    const FDynamicEmitterReplayDataBase& ReplayData = Emitter->GetSource();

                    // TODO: 최적화 필요
                    if (ReplayData.eEmitterType == EDynamicEmitterType::DET_Sprite)
                    {
                        ParticleComponents.Add(Iter);
                    }
                }
            }
        }
    }

    ParticleComponents.Sort(
        [](const UParticleSystemComponent* A, const UParticleSystemComponent* B)
        {
            const FVector LocA = A->GetComponentLocation();
            const FVector LocB = B->GetComponentLocation();
            const FVector LocCam = GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->GetCameraLocation();

            const float DistA = (LocCam - LocA).SquaredLength();
            const float DistB = (LocCam - LocB).SquaredLength();

            return DistA > DistB;
        }
    );
}

void FParticleSpriteRenderPass::ClearRenderArr()
{
    ParticleComponents.Empty();
}

void FParticleSpriteRenderPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    PrepareRender(Viewport);

    DrawParticles();

    CleanUpRender(Viewport);
}

void FParticleSpriteRenderPass::PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    Graphics->DeviceContext->RSSetViewports(1, &Viewport->GetViewportResource()->GetD3DViewport());

    FViewportResource* ViewportResource = Viewport->GetViewportResource();
    if (!ViewportResource)
    {
        return;
    }

    const FRenderTargetRHI* RenderTargetRHI = ViewportResource->GetRenderTarget(EResourceType::ERT_Translucent);
    const FDepthStencilRHI* DepthStencilRHI = ViewportResource->GetDepthStencil(EResourceType::ERT_Scene);

    Graphics->DeviceContext->OMSetRenderTargets(1, &RenderTargetRHI->RTV, DepthStencilRHI->DSV);

    Graphics->DeviceContext->OMSetBlendState(Graphics->BlendState_AlphaBlend, nullptr, 0xffffffff);
    Graphics->DeviceContext->OMSetDepthStencilState(Graphics->DepthStencilState_DepthWriteDisabled, 1);

    Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    ID3D11VertexShader* VertexShader = ShaderManager->GetVertexShaderByKey(L"ParticleSpriteVertexShader");
    ID3D11PixelShader* PixelShader = ShaderManager->GetPixelShaderByKey(L"ParticleSpritePixelShader");
    Graphics->DeviceContext->VSSetShader(VertexShader, nullptr, 0);
    Graphics->DeviceContext->PSSetShader(PixelShader, nullptr, 0);
    Graphics->DeviceContext->IASetInputLayout(nullptr);

    BufferManager->BindStructuredBufferSRV("ParticleSpriteInstanceBuffer", 1, EShaderStage::Vertex);

    BufferManager->BindConstantBuffer(TEXT("FObjectConstantBuffer"), 12, EShaderStage::Vertex);

    BufferManager->BindConstantBuffer(TEXT("FMaterialConstants"), 0, EShaderStage::Pixel);
    BufferManager->BindConstantBuffer(TEXT("FSubUVConstant"), 1, EShaderStage::Pixel);
}

void FParticleSpriteRenderPass::CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);

    Graphics->DeviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    Graphics->DeviceContext->OMSetDepthStencilState(Graphics->DepthStencilState_Default, 1);
}

void FParticleSpriteRenderPass::DrawParticles()
{
    for (const auto PSC : ParticleComponents)
    {
        FParticleDynamicData* Particle = PSC->GetParticleDynamicData();
        if (Particle)
        {
            //UpdateObjectConstant(PSC->GetWorldMatrix(), FVector4(), false);
            UpdateObjectConstant(FMatrix::Identity, FVector4(), false);

            for (auto Emitter : Particle->DynamicEmitterDataArray)
            {
                const FDynamicEmitterReplayDataBase& ReplayData = Emitter->GetSource();

                if (ReplayData.eEmitterType == EDynamicEmitterType::DET_Sprite)
                {
                    FDynamicSpriteEmitterDataBase* SpriteData = dynamic_cast<FDynamicSpriteEmitterDataBase*>(Emitter);
                    if (SpriteData)
                    {
                        const FDynamicSpriteEmitterReplayDataBase* SpriteParticleData = SpriteData->GetSourceData();
                        ProcessParticles(SpriteParticleData);
                    }
                }
            }
        }
    }
}

void FParticleSpriteRenderPass::ProcessParticles(const FDynamicSpriteEmitterReplayDataBase* ReplayData)
{
    // ReplayData
    if (ReplayData == nullptr)
    {
        return;
    }

    TArray<FParticleSpriteVertex> SpriteVertices;

    const uint8* ParticleData = ReplayData->DataContainer.ParticleData;
    const int32 ParticleStride = ReplayData->ParticleStride;

    const int32 SubImages_Horizontal = ReplayData->SubImages_Horizontal;
    const int32 SubImages_Vertical = ReplayData->SubImages_Vertical;
    const int32 SubUVDataOffset = ReplayData->SubUVDataOffset;

    for (int32 i = 0; i < ReplayData->ActiveParticleCount; i++)
    {
        const uint8* ParticleBase = ParticleData + i * ParticleStride;

        DECLARE_PARTICLE_CONST(Particle, ParticleBase)
        FParticleSpriteVertex SpriteVertex = {};
        SpriteVertex.Position = Particle.Location;
        SpriteVertex.Color = Particle.Color;
        SpriteVertex.Size = FVector2D(Particle.Size.X, Particle.Size.Y);
        SpriteVertex.Rotation = Particle.Rotation;
        if (SubUVDataOffset == 0)
        {
            SpriteVertex.SubImageIndex = 0;
        }
        else
        {
            SpriteVertex.SubImageIndex = reinterpret_cast<const FFullSubUVPayload*>(ParticleBase + SubUVDataOffset)->ImageIndex;
        }

        SpriteVertices.Add(SpriteVertex);
    }

    SpriteVertices.Sort(
        [](const FParticleSpriteVertex& A, const FParticleSpriteVertex& B)
        {
            const FVector LocA = A.Position;
            const FVector LocB = B.Position;
            const FVector LocCam = GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->GetCameraLocation();

            const float DistA = (LocCam - LocA).SquaredLength();
            const float DistB = (LocCam - LocB).SquaredLength();

            return DistA > DistB;
        });

    BufferManager->UpdateStructuredBuffer("ParticleSpriteInstanceBuffer", SpriteVertices);

    if (UMaterial* Material = ReplayData->MaterialInterface)
    {
        const FMaterialInfo& MaterialInfo = Material->GetMaterialInfo();

        MaterialUtils::UpdateMaterial(BufferManager, Graphics, MaterialInfo);
    }

    const float SubUVScale_Horizontal = 1.0f / static_cast<float>(SubImages_Horizontal);
    const float SubUVScale_Vertical = 1.0f / static_cast<float>(SubImages_Vertical);

    FSubUVConstant SubUVConstant = {
        FVector2D(0.0f, 0.0f),
        FVector2D(SubUVScale_Horizontal, SubUVScale_Vertical)
    };
    BufferManager->UpdateConstantBuffer(TEXT("FSubUVConstant"), SubUVConstant);

    Graphics->DeviceContext->DrawInstanced(6, SpriteVertices.Num(), 0, 0);
}
