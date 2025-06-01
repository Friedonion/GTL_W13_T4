#include "CompositingPass.h"

#include <array>

#include "Define.h"
#include "RendererHelpers.h"
#include "UnrealClient.h"
#include "UnrealEd/EditorViewportClient.h"

void FCompositingPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManage)
{
    FRenderPassBase::Initialize(InBufferManager, InGraphics, InShaderManage);
    
    ShaderManager->AddPixelShader(L"Compositing", L"Shaders/CompositingShader.hlsl", "main");

    ViewModeBuffer = BufferManager->GetConstantBuffer("FViewModeConstants");

    UINT DiffuseMultiplierSize = sizeof(FGammaConstants);
    BufferManager->CreateBufferGeneric<FGammaConstants>("FGammaConstants", nullptr, DiffuseMultiplierSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
}

void FCompositingPass::PrepareRenderArr()
{
}

void FCompositingPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    // Setup
    FViewportResource* ViewportResource = Viewport->GetViewportResource();
    if (!ViewportResource)
    {
        return;
    }

    const EResourceType ResourceType = EResourceType::ERT_Compositing; 
    FRenderTargetRHI* RenderTargetRHI = Viewport->GetViewportResource()->GetRenderTarget(ResourceType);

    Graphics->DeviceContext->RSSetViewports(1, &Viewport->GetD3DViewport());

    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_Scene), 1, &ViewportResource->GetRenderTarget(EResourceType::ERT_Scene)->SRV);
    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_Translucent), 1, &ViewportResource->GetRenderTarget(EResourceType::ERT_Translucent)->SRV);
    // TODO: 포스트 프로세싱 결과에 Depth of Field 결과만 전달해주고 있음. Fog는 고려하지 않은 방식으로, 추후 개선 필요.
    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_PostProcess), 1, &ViewportResource->GetRenderTarget(EResourceType::ERT_DepthOfField_Result)->SRV);
    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_Editor), 1, &ViewportResource->GetRenderTarget(EResourceType::ERT_Editor)->SRV);
    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_EditorOverlay), 1, &ViewportResource->GetRenderTarget(EResourceType::ERT_EditorOverlay)->SRV);
    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_CameraEffect), 1, &ViewportResource->GetRenderTarget(EResourceType::ERT_PP_CameraEffect)->SRV);
    
    Graphics->DeviceContext->OMSetRenderTargets(1, &RenderTargetRHI->RTV, nullptr);
    Graphics->DeviceContext->ClearRenderTargetView(RenderTargetRHI->RTV, ViewportResource->GetClearColor(ResourceType).data());

    Graphics->DeviceContext->RSSetState(Graphics->RasterizerSolidBack);
    Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    Graphics->DeviceContext->OMSetBlendState(Graphics->BlendState_AlphaBlend, nullptr, 0xffffffff);

    // 버퍼 바인딩
    Graphics->DeviceContext->PSSetConstantBuffers(0, 1, &ViewModeBuffer);
    
    // Update Constant Buffer
    FViewModeConstants ViewModeConstantData = {};
    ViewModeConstantData.ViewMode = static_cast<uint32>(Viewport->GetViewMode());
    BufferManager->UpdateConstantBuffer<FViewModeConstants>("FViewModeConstants", ViewModeConstantData);

    BufferManager->BindConstantBuffer(TEXT("FGammaConstants"), 1, EShaderStage::Pixel);
    
    FGammaConstants GammaConstantData = {};
    GammaConstantData.GammaValue = GammaValue;
    BufferManager->UpdateConstantBuffer<FGammaConstants>("FGammaConstants", GammaConstantData);

    // Render
    ID3D11VertexShader* VertexShader = ShaderManager->GetVertexShaderByKey(L"FullScreenQuadVertexShader");
    ID3D11PixelShader* PixelShader = ShaderManager->GetPixelShaderByKey(L"Compositing");
    Graphics->DeviceContext->VSSetShader(VertexShader, nullptr, 0);
    Graphics->DeviceContext->PSSetShader(PixelShader, nullptr, 0);
    Graphics->DeviceContext->IASetInputLayout(nullptr);
    Graphics->DeviceContext->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
    
    Graphics->DeviceContext->Draw(6, 0);

    // Finish
    Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);

    // Clear
    ID3D11ShaderResourceView* NullSRV[1] = { nullptr };
    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_Scene), 1, NullSRV);
    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_Translucent), 1, NullSRV);
    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_PostProcess), 1, NullSRV);
    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_Editor), 1, NullSRV);
    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_EditorOverlay), 1, NullSRV);
    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_CameraEffect), 1, NullSRV);
    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_Debug), 1, NullSRV);
}

void FCompositingPass::ClearRenderArr()
{
}

void FCompositingPass::PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
}

void FCompositingPass::CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
}
