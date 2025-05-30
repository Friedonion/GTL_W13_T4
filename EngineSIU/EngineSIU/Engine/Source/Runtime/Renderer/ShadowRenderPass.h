#pragma once
#include "RenderPassBase.h"
#include "EngineBaseTypes.h"
#include "Container/Set.h"
#include "Define.h"
#include "UnrealClient.h" // Depth Stencil View
#include <d3d11.h>

#include "Components/Light/PointLightComponent.h"


// ShadowMap을 생성하기 위한 Render Pass입니다.
// Depth 값만을 추출하는 Vertex Shader Only 패스
// 모든 Light에 대한 Depth Map Texture를 생성합니다.
// ViewMode != Unlit일 때에만 Static Mesh Render Pass 에서 실행됩니다

struct FStaticMeshRenderData;
class FDXDBufferManager;
class FDXDShaderManager;
class FGraphicsDevice;
class ULightComponentBase;

class FShadowRenderPass : public FRenderPassBase
{
public:
    FShadowRenderPass() = default;
    virtual ~FShadowRenderPass() override = default;
    
    void PrepareCubeMapRenderState();
    void UpdateCubeMapConstantBuffer(UPointLightComponent*& PointLight, const FMatrix& WorldMatrix) const;
    void RenderCubeMap(const std::shared_ptr<FEditorViewportClient>& Viewport, UPointLightComponent*& PointLight);
    void SetLightData(const TArray<class UPointLightComponent*>& InPointLights, const TArray<class USpotLightComponent*>& InSpotLights);

    virtual void Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager) override;
    void InitializeShadowManager(class FShadowManager* InShadowManager);
    void PrepareRenderState();
    void PrepareCSMRenderState();
    virtual void PrepareRenderArr() override;
    void UpdateIsShadowConstant(int32 IsShadow) const;
    void Render(ULightComponentBase* Light);
    virtual void Render(const std::shared_ptr<FEditorViewportClient>& Viewport) override;    
    virtual void ClearRenderArr() override;

    void RenderPrimitive(FStaticMeshRenderData* RenderData, const TArray<FStaticMaterial*> Materials, TArray<UMaterial*> OverrideMaterials, int32 SelectedSubMeshIndex);
    virtual void RenderAllStaticMeshes(const std::shared_ptr<FEditorViewportClient>& Viewport);
    void RenderAllStaticMeshesForCSM(const std::shared_ptr<FEditorViewportClient>& Viewport,
                                     FCascadeConstantBuffer FCasCadeData);
    void BindResourcesForSampling();

    void RenderAllStaticMeshesForPointLight(const std::shared_ptr<FEditorViewportClient>& Viewport, UPointLightComponent*& PointLight);

protected:
    virtual void PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport) override;
    virtual void CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport) override;
    
    virtual void CreateResource() override;
    
private:
    TArray<class UStaticMeshComponent*> StaticMeshComponents;
    TArray<UPointLightComponent*> PointLights;
    TArray<USpotLightComponent*> SpotLights;
    
    FShadowManager* ShadowManager = nullptr;

    ID3D11InputLayout* StaticMeshIL = nullptr;
    ID3D11VertexShader* DepthOnlyVS = nullptr;
    ID3D11PixelShader* DepthOnlyPS = nullptr;
    ID3D11SamplerState* Sampler = nullptr;

    ID3D11VertexShader* DepthCubeMapVS = nullptr;
    ID3D11GeometryShader* DepthCubeMapGS = nullptr;


    ID3D11VertexShader* CascadedShadowMapVS = nullptr;
    ID3D11GeometryShader* CascadedShadowMapGS = nullptr;
    ID3D11PixelShader* CascadedShadowMapPS = nullptr;

    D3D11_VIEWPORT ShadowViewport;


    uint32 ShadowMapWidth = 2048;
    uint32 ShadowMapHeight = 2048;

    FLOAT ClearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
};
