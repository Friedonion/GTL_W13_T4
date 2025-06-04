#pragma once
#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

#define _TCHAR_DEFINED
#include <d3d11.h>
#include <d3dcompiler.h>

#include "EngineBaseTypes.h"
#include "Define.h"
#include "Container/Set.h"

#include "D3D11RHI/GraphicDevice.h"
#include "D3D11RHI/DXDBufferManager.h"


class FPostProcessRenderPass;
class FParticleMeshRenderPass;
class FParticleSpriteRenderPass;
class FTranslucentRenderPass;
class IRenderPass;
class FLightHeatMapRenderPass;
class FPostProcessCompositingPass;
enum class EResourceType : uint8;

class UWorld;
class UObject;

class FDXDShaderManager;
class FEditorViewportClient;

class FViewportResource;

class FOpaqueRenderPass;
class FWorldBillboardRenderPass;
class FEditorBillboardRenderPass;
class FGizmoRenderPass;
class FUpdateLightBufferPass;
class FDepthBufferDebugPass;
class FLineRenderPass;
class FFogRenderPass;
class FCameraEffectRenderPass;
class FCompositingPass;
class FSlateRenderPass;
class FEditorRenderPass;
class FDepthPrePass;
class FTileLightCullingPass;
class FGPUTimingManager;

class FRenderer
{
public:
    //==========================================================================
    // 초기화/해제 관련 함수
    //==========================================================================
    void Initialize(FGraphicsDevice* InGraphics, FDXDBufferManager* InBufferManager, FGPUTimingManager* InGPUTimingManager);
    void Release();

    //==========================================================================
    // 렌더 패스 관련 함수
    //==========================================================================
    void Render(const std::shared_ptr<FEditorViewportClient>& Viewport);
    void RenderViewport(const std::shared_ptr<FEditorViewportClient>& Viewport) const; // TODO: 추후 RenderSlate로 변경해야함

protected:
    void BeginRender(const std::shared_ptr<FEditorViewportClient>& Viewport) const;
    void UpdateCommonBuffer(const std::shared_ptr<FEditorViewportClient>& Viewport) const;
    void PrepareRender(FViewportResource* ViewportResource) const;
    void PrepareRenderPass() const;

    void RenderPreScene(const std::shared_ptr<FEditorViewportClient>& Viewport) const;
    void RenderOpaque(const std::shared_ptr<FEditorViewportClient>& Viewport) const;
    void RenderEditorDepthElement(const std::shared_ptr<FEditorViewportClient>& Viewport) const;
    void RenderTranslucent(const std::shared_ptr<FEditorViewportClient>& Viewport) const;
    void RenderEditorOverlay(const std::shared_ptr<FEditorViewportClient>& Viewport) const;
    void RenderPostProcess(const std::shared_ptr<FEditorViewportClient>& Viewport) const;
    void RenderFinalResult(const std::shared_ptr<FEditorViewportClient>& Viewport) const;
    
    void EndRender() const;
    void ClearRenderArr() const;
    
    //==========================================================================
    // 버퍼 생성/해제 함수 (템플릿 포함)
    //==========================================================================
public:
    // 상수 버퍼 생성/해제
    void CreateConstantBuffers();
    void ReleaseConstantBuffer() const;

    void CreateCommonShader() const;

public:
    FGraphicsDevice* Graphics;
    FDXDBufferManager* BufferManager;
    FDXDShaderManager* ShaderManager = nullptr;
    class FShadowManager* ShadowManager = nullptr;
    FGPUTimingManager* GPUTimingManager = nullptr;
    
    class FShadowRenderPass* ShadowRenderPass;

    FOpaqueRenderPass* OpaqueRenderPass = nullptr;
    FWorldBillboardRenderPass* WorldBillboardRenderPass = nullptr;
    FEditorBillboardRenderPass* EditorBillboardRenderPass = nullptr;
    FGizmoRenderPass* GizmoRenderPass = nullptr;
    FUpdateLightBufferPass* UpdateLightBufferPass = nullptr;
    FLineRenderPass* LineRenderPass = nullptr;
    FEditorRenderPass* EditorRenderPass = nullptr;
    FTranslucentRenderPass* TranslucentRenderPass = nullptr;

    FParticleSpriteRenderPass* ParticleSpriteRenderPass = nullptr;
    FParticleMeshRenderPass* ParticleMeshRenderPass = nullptr;
    
    FDepthPrePass* DepthPrePass = nullptr;
    FTileLightCullingPass* TileLightCullingPass = nullptr;

    FPostProcessRenderPass* PostProcessRenderPass = nullptr;

    FCompositingPass* CompositingPass = nullptr;
    
    FSlateRenderPass* SlateRenderPass = nullptr;

private:
    template <typename RenderPassType>
        requires std::derived_from<RenderPassType, IRenderPass>
    RenderPassType* AddRenderPass();

    TArray<IRenderPass*> RenderPasses;
    
    const int32 MaxBoneNum = 1024;
    const int32 MaxParticleInstanceNum = 1024;

public:
    static constexpr int32 MaxNumInstances = 128;

    bool bSkeletalMeshInstanced = true;
};

template <typename RenderPassType> requires std::derived_from<RenderPassType, IRenderPass>
RenderPassType* FRenderer::AddRenderPass()
{
    RenderPassType* RenderPass = new RenderPassType();
    RenderPasses.Add(RenderPass);
    return RenderPass;
}
