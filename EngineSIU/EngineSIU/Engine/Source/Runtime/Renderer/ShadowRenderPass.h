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
    
    void PreparePointLightRenderState();
    void UpdateCubeMapConstantBuffer(UPointLightComponent*& PointLight, const FMatrix& WorldMatrix) const;
    void SetLightData(const TArray<class UPointLightComponent*>& InPointLights, const TArray<class USpotLightComponent*>& InSpotLights);

    virtual void Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager) override;
    void InitializeShadowManager(class FShadowManager* InShadowManager);
    void PrepareSpotLightRenderState();
    void PrepareCSMRenderState();
    virtual void PrepareRenderArr() override;
    void UpdateIsShadowConstant(int32 IsShadow) const;
    virtual void Render(const std::shared_ptr<FEditorViewportClient>& Viewport) override;    
    virtual void ClearRenderArr() override;

    virtual void RenderAllMeshesForSpotLight(const std::shared_ptr<FEditorViewportClient>& Viewport);
    void RenderAllMeshesForCSM(const std::shared_ptr<FEditorViewportClient>& Viewport,
                                     FCascadeConstantBuffer FCasCadeData);

    void BindResourcesForSampling();

    void RenderAllMeshesForPointLight(const std::shared_ptr<FEditorViewportClient>& Viewport, UPointLightComponent*& PointLight);

protected:
    virtual void PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport) override;
    virtual void CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport) override;
    
    virtual void CreateResource() override;
    
private:
    TArray<class UStaticMeshComponent*> StaticMeshComponents;
    TArray<class USkeletalMeshComponent*> SkeletalMeshComponents;
    
    TArray<UPointLightComponent*> PointLights;
    TArray<USpotLightComponent*> SpotLights;
    
    FShadowManager* ShadowManager;

    ID3D11InputLayout* StaticMeshIL;
    ID3D11VertexShader* DepthOnlyVS;
    ID3D11SamplerState* Sampler;

    ID3D11VertexShader* DepthCubeMapVS;
    ID3D11GeometryShader* DepthCubeMapGS;

    ID3D11VertexShader* CascadedShadowMapVS;
    ID3D11GeometryShader* CascadedShadowMapGS;
    ID3D11PixelShader* CascadedShadowMapPS;

    D3D11_VIEWPORT ShadowViewport;
    
    uint32 ShadowMapWidth = 2048;
    uint32 ShadowMapHeight = 2048;

    FLOAT ClearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
};
