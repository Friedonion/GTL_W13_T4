#include "PostProcessCompositingPass.h"

#include <array>

#include "RendererHelpers.h"
#include "UnrealClient.h"
#include "D3D11RHI/DXDShaderManager.h"
#include "UnrealEd/EditorViewportClient.h"

void FPostProcessCompositingPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManage)
{
    FRenderPassBase::Initialize(InBufferManager, InGraphics, InShaderManage);

    ShaderManager->AddVertexShader(L"PostProcessCompositing", L"Shaders/PostProcessCompositingShader.hlsl", "mainVS");
    ShaderManager->AddPixelShader(L"PostProcessCompositing", L"Shaders/PostProcessCompositingShader.hlsl", "mainPS");
}

void FPostProcessCompositingPass::PrepareRenderArr()
{
}

void FPostProcessCompositingPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    // Setup
    FViewportResource* ViewportResource = Viewport->GetViewportResource();
    if (!ViewportResource)
    {
        return;
    }

    constexpr EResourceType ResourceType = EResourceType::ERT_PostProcessCompositing; 
    FRenderTargetRHI* RenderTargetRHI = Viewport->GetViewportResource()->GetRenderTarget(ResourceType);

    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_Fog), 1, &ViewportResource->GetRenderTarget(EResourceType::ERT_PP_Fog)->SRV);

    Graphics->DeviceContext->OMSetRenderTargets(1, &RenderTargetRHI->RTV, nullptr);

    Graphics->DeviceContext->RSSetState(Graphics->RasterizerSolidBack);
    Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    Graphics->DeviceContext->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
    
    // Render
    ID3D11VertexShader* VertexShader = ShaderManager->GetVertexShaderByKey(L"PostProcessCompositing");
    ID3D11PixelShader* PixelShader = ShaderManager->GetPixelShaderByKey(L"PostProcessCompositing");
    Graphics->DeviceContext->VSSetShader(VertexShader, nullptr, 0);
    Graphics->DeviceContext->PSSetShader(PixelShader, nullptr, 0);
    Graphics->DeviceContext->IASetInputLayout(nullptr);
    Graphics->DeviceContext->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
    
    //Graphics->DeviceContext->Draw(6, 0);

    // Finish
    Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);

    // Clear
    ID3D11ShaderResourceView* NullSRV[1] = { nullptr };
    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_Fog), 1, NullSRV);
}

void FPostProcessCompositingPass::ClearRenderArr()
{
}

void FPostProcessCompositingPass::PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
}

void FPostProcessCompositingPass::CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
}
