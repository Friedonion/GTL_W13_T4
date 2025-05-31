
#include "Renderer.h"

#include <array>
#include "World/World.h"
#include "Engine/EditorEngine.h"
#include "UnrealEd/EditorViewportClient.h"
#include "D3D11RHI/DXDShaderManager.h"
#include "OpaqueRenderPass.h"
#include "WorldBillboardRenderPass.h"
#include "EditorBillboardRenderPass.h"
#include "GizmoRenderPass.h"
#include "UpdateLightBufferPass.h"
#include "LineRenderPass.h"
#include "SlateRenderPass.h"
#include "EditorRenderPass.h"
#include "DepthPrePass.h"
#include "TileLightCullingPass.h"
#include "TranslucentRenderPass.h"

#include "CompositingPass.h"
#include "ParticleHelper.h"
#include "ParticleSpriteRenderPass.h"
#include "ParticleMeshRenderPass.h"
#include "PostProcess/PostProcessRenderPass.h"
#include "ShadowManager.h"
#include "ShadowRenderPass.h"
#include "UnrealClient.h"
#include "GameFrameWork/Actor.h"

#include "PropertyEditor/ShowFlags.h"
#include "Stats/Stats.h"
#include "Stats/GPUTimingManager.h"

//------------------------------------------------------------------------------
// 초기화 및 해제 관련 함수
//------------------------------------------------------------------------------
void FRenderer::Initialize(FGraphicsDevice* InGraphics, FDXDBufferManager* InBufferManager, FGPUTimingManager* InGPUTimingManager)
{
    Graphics = InGraphics;
    BufferManager = InBufferManager;
    GPUTimingManager = InGPUTimingManager;

    ShaderManager = new FDXDShaderManager(Graphics->Device);
    ShadowManager = new FShadowManager();
    ShadowRenderPass = AddRenderPass<FShadowRenderPass>();

    CreateConstantBuffers();
    CreateCommonShader();
    
    OpaqueRenderPass = AddRenderPass<FOpaqueRenderPass>();
    WorldBillboardRenderPass = AddRenderPass<FWorldBillboardRenderPass>();
    EditorBillboardRenderPass = AddRenderPass<FEditorBillboardRenderPass>();
    GizmoRenderPass = AddRenderPass<FGizmoRenderPass>();
    UpdateLightBufferPass = AddRenderPass<FUpdateLightBufferPass>();
    LineRenderPass = AddRenderPass<FLineRenderPass>();
    EditorRenderPass = AddRenderPass<FEditorRenderPass>();
    TranslucentRenderPass = AddRenderPass<FTranslucentRenderPass>();

    ParticleSpriteRenderPass = AddRenderPass<FParticleSpriteRenderPass>();
    ParticleMeshRenderPass = AddRenderPass<FParticleMeshRenderPass>();
    
    DepthPrePass = AddRenderPass<FDepthPrePass>();
    TileLightCullingPass = AddRenderPass<FTileLightCullingPass>();

    PostProcessRenderPass = AddRenderPass<FPostProcessRenderPass>();
    
    CompositingPass = AddRenderPass<FCompositingPass>();
    SlateRenderPass = AddRenderPass<FSlateRenderPass>();

    const bool bShadowManagerInitialized = ShadowManager->Initialize(Graphics, BufferManager);
    assert(bShadowManagerInitialized);

    for (IRenderPass* RenderPass : RenderPasses)
    {
        RenderPass->Initialize(BufferManager, Graphics, ShaderManager);
    }
    ShadowRenderPass->InitializeShadowManager(ShadowManager);
}

void FRenderer::Release()
{
    delete ShaderManager;
    delete ShadowManager;

    for (const IRenderPass* RenderPass : RenderPasses)
    {
        delete RenderPass;
    }
}

//------------------------------------------------------------------------------
// 사용하는 모든 상수 버퍼 생성
//------------------------------------------------------------------------------
void FRenderer::CreateConstantBuffers()
{
    UINT CascadeBufferSize = sizeof(FCascadeConstantBuffer);
    BufferManager->CreateBufferGeneric<FCascadeConstantBuffer>("FCascadeConstantBuffer", nullptr, CascadeBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT PointLightGSBufferSize = sizeof(FPointLightGSBuffer);
    BufferManager->CreateBufferGeneric<FPointLightGSBuffer>("FPointLightGSBuffer", nullptr, PointLightGSBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT IsShadowBufferSize = sizeof(FIsShadowConstants);
    BufferManager->CreateBufferGeneric<FIsShadowConstants>("FIsShadowConstants", nullptr, IsShadowBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT ShadowBufferSize = sizeof(FShadowConstantBuffer);
    BufferManager->CreateBufferGeneric<FShadowConstantBuffer>("FShadowConstantBuffer", nullptr, ShadowBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT ObjectBufferSize = sizeof(FObjectConstantBuffer);
    BufferManager->CreateBufferGeneric<FObjectConstantBuffer>("FObjectConstantBuffer", nullptr, ObjectBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT CameraConstantBufferSize = sizeof(FCameraConstantBuffer);
    BufferManager->CreateBufferGeneric<FCameraConstantBuffer>("FCameraConstantBuffer", nullptr, CameraConstantBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT SubUVBufferSize = sizeof(FSubUVConstant);
    BufferManager->CreateBufferGeneric<FSubUVConstant>("FSubUVConstant", nullptr, SubUVBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT MaterialBufferSize = sizeof(FMaterialConstants);
    BufferManager->CreateBufferGeneric<FMaterialConstants>("FMaterialConstants", nullptr, MaterialBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT SubMeshBufferSize = sizeof(FSubMeshConstants);
    BufferManager->CreateBufferGeneric<FSubMeshConstants>("FSubMeshConstants", nullptr, SubMeshBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT TextureBufferSize = sizeof(FTextureUVConstants);
    BufferManager->CreateBufferGeneric<FTextureUVConstants>("FTextureConstants", nullptr, TextureBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
    
    UINT LitUnlitBufferSize = sizeof(FLitUnlitConstants);
    BufferManager->CreateBufferGeneric<FLitUnlitConstants>("FLitUnlitConstants", nullptr, LitUnlitBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT ViewModeBufferSize = sizeof(FViewModeConstants);
    BufferManager->CreateBufferGeneric<FViewModeConstants>("FViewModeConstants", nullptr, ViewModeBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT FogConstantBufferSize = sizeof(FFogConstants);
    BufferManager->CreateBufferGeneric<FFogConstants>("FFogConstants", nullptr, FogConstantBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT LightInfoBufferSize = sizeof(FLightInfoBuffer);
    BufferManager->CreateBufferGeneric<FLightInfoBuffer>("FLightInfoBuffer", nullptr, LightInfoBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT CPUSkinningBufferSize = sizeof(FCPUSkinningConstants);
    BufferManager->CreateBufferGeneric<FCPUSkinningConstants>("FCPUSkinningConstants", nullptr, CPUSkinningBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    BufferManager->CreateStructuredBufferGeneric<FMatrix>("BoneBuffer", nullptr, MaxBoneNum, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    BufferManager->CreateStructuredBufferGeneric<FParticleSpriteVertex>("ParticleSpriteInstanceBuffer", nullptr, MaxParticleInstanceNum, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    BufferManager->CreateStructuredBufferGeneric<FMeshParticleInstanceVertex>("ParticleMeshInstanceBuffer", nullptr, MaxParticleInstanceNum, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    BufferManager->CreateBufferGeneric<FViewportSize>("FViewportSize", nullptr, sizeof(FViewportSize), D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
    
    // TODO: 함수로 분리
    ID3D11Buffer* ObjectBuffer = BufferManager->GetConstantBuffer(TEXT("FObjectConstantBuffer"));
    ID3D11Buffer* CameraConstantBuffer = BufferManager->GetConstantBuffer(TEXT("FCameraConstantBuffer"));
    Graphics->DeviceContext->VSSetConstantBuffers(12, 1, &ObjectBuffer);
    Graphics->DeviceContext->VSSetConstantBuffers(13, 1, &CameraConstantBuffer);
    Graphics->DeviceContext->PSSetConstantBuffers(12, 1, &ObjectBuffer);
    Graphics->DeviceContext->PSSetConstantBuffers(13, 1, &CameraConstantBuffer);
}

void FRenderer::ReleaseConstantBuffer() const
{
    BufferManager->ReleaseConstantBuffer();
    BufferManager->ReleaseStructuredBuffer();
}

void FRenderer::CreateCommonShader() const
{
    D3D11_INPUT_ELEMENT_DESC StaticMeshLayoutDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    HRESULT hr = ShaderManager->AddVertexShaderAndInputLayout(L"StaticMeshVertexShader", L"Shaders/StaticMeshVertexShader.hlsl", "mainVS", StaticMeshLayoutDesc, ARRAYSIZE(StaticMeshLayoutDesc));
    if (FAILED(hr))
    {
        return;
    }

    D3D11_INPUT_ELEMENT_DESC SkeletalMeshLayoutDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"BONE_INDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"BONE_WEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    hr = ShaderManager->AddVertexShaderAndInputLayout(L"SkeletalMeshVertexShader", L"Shaders/SkeletalMeshVertexShader.hlsl", "mainVS", SkeletalMeshLayoutDesc, ARRAYSIZE(SkeletalMeshLayoutDesc));
    if (FAILED(hr))
    {
        return;
    }
    
#pragma region UberShader
    D3D_SHADER_MACRO DefinesGouraud[] =
    {
        { GOURAUD, "1" },
        { nullptr, nullptr }
    };
    hr = ShaderManager->AddVertexShaderAndInputLayout(L"GOURAUD_StaticMeshVertexShader", L"Shaders/StaticMeshVertexShader.hlsl", "mainVS", StaticMeshLayoutDesc, ARRAYSIZE(StaticMeshLayoutDesc), DefinesGouraud);
    if (FAILED(hr))
    {
        return;
    }
    
    hr = ShaderManager->AddVertexShaderAndInputLayout(L"GOURAUD_SkeletalMeshVertexShader", L"Shaders/SkeletalMeshVertexShader.hlsl", "mainVS", SkeletalMeshLayoutDesc, ARRAYSIZE(SkeletalMeshLayoutDesc), DefinesGouraud);
    if (FAILED(hr))
    {
        return;
    }
#pragma endregion UberShader

    hr = ShaderManager->AddVertexShader(L"FullScreenQuadVertexShader", L"Shaders/FullScreenQuadVertexShader.hlsl", "main");
    if (FAILED(hr))
    {
        return;
    }
}

void FRenderer::PrepareRender(FViewportResource* ViewportResource) const
{
    // Setup Viewport
    Graphics->DeviceContext->RSSetViewports(1, &ViewportResource->GetD3DViewport());

    ViewportResource->ClearDepthStencils(Graphics->DeviceContext);
    ViewportResource->ClearRenderTargets(Graphics->DeviceContext);

    PrepareRenderPass();

    Graphics->DeviceContext->PSSetSamplers(0, 1, &Graphics->SamplerState_LinearWrap);
    Graphics->DeviceContext->PSSetSamplers(1, 1, &Graphics->SamplerState_LinearClamp);
    Graphics->DeviceContext->PSSetSamplers(2, 1, &Graphics->SamplerState_PointWrap);
    Graphics->DeviceContext->PSSetSamplers(3, 1, &Graphics->SamplerState_PointClamp);

    Graphics->DeviceContext->VSSetSamplers(0, 1, &Graphics->SamplerState_LinearWrap);
}

void FRenderer::PrepareRenderPass() const
{
    for (IRenderPass* RenderPass : RenderPasses)
    {
        RenderPass->PrepareRenderArr();
    }
}

void FRenderer::ClearRenderArr() const
{
    for (IRenderPass* RenderPass : RenderPasses)
    {
        RenderPass->ClearRenderArr();
    }
}

void FRenderer::UpdateCommonBuffer(const std::shared_ptr<FEditorViewportClient>& Viewport) const
{
    FCameraConstantBuffer CameraConstantBuffer;
    CameraConstantBuffer.ViewMatrix = Viewport->GetViewMatrix();
    CameraConstantBuffer.InvViewMatrix = FMatrix::Inverse(CameraConstantBuffer.ViewMatrix);
    CameraConstantBuffer.ProjectionMatrix = Viewport->GetProjectionMatrix();
    CameraConstantBuffer.InvProjectionMatrix = FMatrix::Inverse(CameraConstantBuffer.ProjectionMatrix);
    CameraConstantBuffer.ViewLocation = Viewport->GetCameraLocation();
    CameraConstantBuffer.NearClip = Viewport->GetCameraNearClip();
    CameraConstantBuffer.FarClip = Viewport->GetCameraFarClip();
    BufferManager->UpdateConstantBuffer("FCameraConstantBuffer", CameraConstantBuffer);
}

void FRenderer::BeginRender(const std::shared_ptr<FEditorViewportClient>& Viewport) const
{
    FViewportResource* ViewportResource = Viewport->GetViewportResource();
    if (!ViewportResource)
    {
        return;
    }

    UpdateCommonBuffer(Viewport);
    
    PrepareRender(ViewportResource);
}

void FRenderer::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    if (!GPUTimingManager || !GPUTimingManager->IsInitialized())
    {
        return;
    }
    
    /**
     * 각 렌더 패스의 시작과 끝은 필요한 리소스를 바인딩하고 해제하는 것까지입니다.
     * 다음에 작동할 렌더 패스에서는 이전에 사용했던 리소스들을 충돌 없이 바인딩 할 수 있어야 한다는 의미입니다.
     * e.g.
     *   1번 렌더 패스: 여기에서 사용했던 RTV를 마지막에 해제함으로써, 해당 RTV와 연결된 텍스처를 쉐이더 리소스로 사용할 수 있습니다.
     *   2번 렌더 패스: 1번 렌더 패스에서 렌더한 결과 텍스처를 쉐이더 리소스로 사용할 수 있습니다.
     *
     * 경우에 따라(연속적인 렌더 패스에서 동일한 리소스를 사용하는 경우) 바인딩 및 해제 작업을 생략하는 것도 가능하지만,
     * 다음 전제 조건을 지켜주어야 합니다.
     *   1. 렌더 패스는 엄격하게 순차적으로 실행됨
     *   2. 렌더 타겟의 생명주기와 용도가 명확함
     *   3. RTV -> SRV 전환 타이밍이 정확히 지켜짐
     */
    
    const uint64 ShowFlag = Viewport->GetShowFlag();
    const EViewModeIndex ViewMode = Viewport->GetViewMode();

    QUICK_SCOPE_CYCLE_COUNTER(Renderer_Render_CPU)
    QUICK_GPU_SCOPE_CYCLE_COUNTER(Renderer_Render_GPU, *GPUTimingManager)

    BeginRender(Viewport);
    
    RenderPreScene(Viewport);
    RenderOpaque(Viewport);
    RenderEditorDepthElement(Viewport);
    RenderTranslucent(Viewport);
    RenderEditorOverlay(Viewport);
    RenderPostProcess(Viewport);

    RenderFinalResult(Viewport);

    EndRender();
}

void FRenderer::RenderPreScene(const std::shared_ptr<FEditorViewportClient>& Viewport) const
{
    const uint64 ShowFlag = Viewport->GetShowFlag();
    if (ShowFlag & (EEngineShowFlags::SF_Primitives | EEngineShowFlags::SF_SkeletalMesh))
    {
        if (DepthPrePass) // Depth Pre Pass : 렌더타겟 nullptr 및 렌더 후 복구
        {
            QUICK_SCOPE_CYCLE_COUNTER(DepthPrePass_CPU)
            QUICK_GPU_SCOPE_CYCLE_COUNTER(DepthPrePass_GPU, *GPUTimingManager)
            DepthPrePass->Render(Viewport);
        }

        // Added Compute Shader Pass
        if (TileLightCullingPass)
        {
            {
                QUICK_SCOPE_CYCLE_COUNTER(TileLightCulling_CPU)
                QUICK_GPU_SCOPE_CYCLE_COUNTER(TileLightCulling_GPU, *GPUTimingManager)
                TileLightCullingPass->Render(Viewport);
            }

            // 이후 패스에서 사용할 수 있도록 리소스 생성
            // @todo UpdateLightBuffer에서 병목 발생 -> 필요한 라이트에 대하여만 업데이트 필요, Tiled Culling으로 GPU->CPU 전송은 주객전도
            UpdateLightBufferPass->SetLightData(
                TileLightCullingPass->GetPointLights(),
                TileLightCullingPass->GetSpotLights(),
                TileLightCullingPass->GetPerTilePointLightIndexMaskBufferSRV(),
                TileLightCullingPass->GetPerTileSpotLightIndexMaskBufferSRV()
            );

            {
                QUICK_SCOPE_CYCLE_COUNTER(UpdateLightBufferPass_CPU)
                QUICK_GPU_SCOPE_CYCLE_COUNTER(UpdateLightBufferPass_GPU, *GPUTimingManager)
                UpdateLightBufferPass->Render(Viewport);
            }
        }

        if (Viewport->GetViewMode() != EViewModeIndex::VMI_Unlit)
        {
            ShadowRenderPass->SetLightData(TileLightCullingPass->GetPointLights(), TileLightCullingPass->GetSpotLights());
            {
                QUICK_SCOPE_CYCLE_COUNTER(ShadowPass_CPU)
                QUICK_GPU_SCOPE_CYCLE_COUNTER(ShadowPass_GPU, *GPUTimingManager)
                ShadowRenderPass->Render(Viewport);
            }
            ShadowManager->BindResourcesForSampling();
        }
    }
}

void FRenderer::RenderOpaque(const std::shared_ptr<FEditorViewportClient>& Viewport) const
{
    const uint64 ShowFlag = Viewport->GetShowFlag();
    
    if (ShowFlag & (EEngineShowFlags::SF_Primitives | EEngineShowFlags::SF_SkeletalMesh))
    {
        {
            QUICK_SCOPE_CYCLE_COUNTER(OpaquePass_CPU)
            QUICK_GPU_SCOPE_CYCLE_COUNTER(OpaquePass_GPU, *GPUTimingManager)
            OpaqueRenderPass->Render(Viewport);
        }
    }
    
    if (ShowFlag & EEngineShowFlags::SF_Particles)
    {
        {
            QUICK_SCOPE_CYCLE_COUNTER(ParticleMeshPass_CPU)
            QUICK_GPU_SCOPE_CYCLE_COUNTER(ParticleMeshPass_GPU, *GPUTimingManager)
            ParticleMeshRenderPass->Render(Viewport);
        }
    }
}

void FRenderer::RenderEditorDepthElement(const std::shared_ptr<FEditorViewportClient>& Viewport) const
{
    if (GEngine->ActiveWorld->WorldType != EWorldType::PIE)
    {
        {
            QUICK_SCOPE_CYCLE_COUNTER(EditorRenderPass_CPU)
            QUICK_GPU_SCOPE_CYCLE_COUNTER(EditorRenderPass_GPU, *GPUTimingManager)
            EditorRenderPass->Render(Viewport); // TODO: 임시로 이전에 작성되었던 와이어 프레임 렌더 패스이므로, 이후 개선 필요.
        }
        {
            QUICK_SCOPE_CYCLE_COUNTER(LinePass_CPU)
            QUICK_GPU_SCOPE_CYCLE_COUNTER(LinePass_GPU, *GPUTimingManager)
            LineRenderPass->Render(Viewport); // 기존 뎁스를 그대로 사용하지만 뎁스를 클리어하지는 않음
        }
    }
}

void FRenderer::RenderTranslucent(const std::shared_ptr<FEditorViewportClient>& Viewport) const
{
    const uint64 ShowFlag = Viewport->GetShowFlag();
    
    if (ShowFlag & (EEngineShowFlags::SF_Primitives | EEngineShowFlags::SF_SkeletalMesh))
    {
        {
            QUICK_SCOPE_CYCLE_COUNTER(TranslucentPass_CPU)
            QUICK_GPU_SCOPE_CYCLE_COUNTER(TranslucentPass_GPU, *GPUTimingManager)
            TranslucentRenderPass->Render(Viewport);
        }
    }

    if (ShowFlag & EEngineShowFlags::SF_Particles)
    {
        {
            QUICK_SCOPE_CYCLE_COUNTER(ParticleSpritePass_CPU)
            QUICK_GPU_SCOPE_CYCLE_COUNTER(ParticleSpritePass_GPU, *GPUTimingManager)
            ParticleSpriteRenderPass->Render(Viewport);
        }
    }
    
    if (ShowFlag & EEngineShowFlags::SF_BillboardText)
    {
        {
            // Render World Billboard
            QUICK_SCOPE_CYCLE_COUNTER(WorldBillboardPass_CPU)
            QUICK_GPU_SCOPE_CYCLE_COUNTER(WorldBillboardPass_GPU, *GPUTimingManager)
            WorldBillboardRenderPass->Render(Viewport);
        }
        if (GEngine->ActiveWorld->WorldType != EWorldType::PIE)
        {
            // Render Editor Billboard
            QUICK_SCOPE_CYCLE_COUNTER(EditorBillboardPass_CPU)
            QUICK_GPU_SCOPE_CYCLE_COUNTER(EditorBillboardPass_GPU, *GPUTimingManager)
            EditorBillboardRenderPass->Render(Viewport);
        }
    }
}

void FRenderer::RenderEditorOverlay(const std::shared_ptr<FEditorViewportClient>& Viewport) const
{
    if (GEngine->ActiveWorld->WorldType != EWorldType::PIE)
    {
        {
            QUICK_SCOPE_CYCLE_COUNTER(GizmoPass_CPU)
            QUICK_GPU_SCOPE_CYCLE_COUNTER(GizmoPass_GPU, *GPUTimingManager)
            GizmoRenderPass->Render(Viewport); // 기존 뎁스를 SRV로 전달해서 샘플 후 비교하기 위해 기즈모 전용 DSV 사용
        }
    }
}

void FRenderer::RenderPostProcess(const std::shared_ptr<FEditorViewportClient>& Viewport) const
{
    const EViewModeIndex ViewMode = Viewport->GetViewMode();

    if (ViewMode < EViewModeIndex::VMI_Unlit)
    {
        PostProcessRenderPass->Render(Viewport);
    }
}

void FRenderer::RenderFinalResult(const std::shared_ptr<FEditorViewportClient>& Viewport) const
{
    {
        // Compositing: 위에서 렌더한 결과들을 하나로 합쳐서 뷰포트의 최종 이미지를 만드는 작업
        QUICK_SCOPE_CYCLE_COUNTER(CompositingPass_CPU)
        QUICK_GPU_SCOPE_CYCLE_COUNTER(CompositingPass_GPU, *GPUTimingManager)
        CompositingPass->Render(Viewport);
    }
}

void FRenderer::EndRender() const
{
    ClearRenderArr();
    ShaderManager->ReloadAllShaders(); 
}

void FRenderer::RenderViewport(const std::shared_ptr<FEditorViewportClient>& Viewport) const
{
    QUICK_SCOPE_CYCLE_COUNTER(SlatePass_CPU)
    QUICK_GPU_SCOPE_CYCLE_COUNTER(SlatePass_GPU, *GPUTimingManager)
    SlateRenderPass->Render(Viewport);
}
