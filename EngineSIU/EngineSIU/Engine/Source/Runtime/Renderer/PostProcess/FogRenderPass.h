#pragma once
#define _TCHAR_DEFINED
#include <d3d11.h>
#include <memory>

#include "RenderPassBase.h"
#include "Engine/Classes/Components/HeightFogComponent.h"

class FGraphicsDevice;
class FDXDShaderManager;
class FDXDBufferManager;
class FEditorViewportClient;

class FFogRenderPass : public FRenderPassBase
{
public:
    FFogRenderPass() = default;
    virtual ~FFogRenderPass() override = default;

    virtual void Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManage) override;
    
    virtual void PrepareRenderArr() override;

    virtual void Render(const std::shared_ptr<FEditorViewportClient>& Viewport) override;

    virtual void ClearRenderArr() override;

    void PrepareRenderState();

    void UpdateFogConstant(UHeightFogComponent* Fog);

private:
    virtual void PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport) override;
    virtual void CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport) override;
    
    // Fog 렌더링용 셰이더 생성 및 입력 레이아웃 설정
    virtual void CreateResource() override;
    void UpdateShader();

    ID3D11VertexShader* VertexShader = nullptr;
    ID3D11PixelShader* PixelShader = nullptr;

    TArray<UHeightFogComponent*> FogComponents;
};
