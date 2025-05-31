#include "DepthOfFieldRenderPass.h"

#include "RendererHelpers.h"
#include "ShaderConstants.h"
#include "UnrealClient.h"
#include "D3D11RHI/DXDShaderManager.h"
#include "Engine/Engine.h"
#include "UnrealEd/EditorViewportClient.h"
#include "World/World.h"

void FDepthOfFieldRenderPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManage)
{
    FRenderPassBase::Initialize(InBufferManager, InGraphics, InShaderManage);
}

void FDepthOfFieldRenderPass::PrepareRenderArr()
{
    FRenderPassBase::PrepareRenderArr();
}

void FDepthOfFieldRenderPass::ClearRenderArr()
{
    FRenderPassBase::ClearRenderArr();
}

void FDepthOfFieldRenderPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    FViewportResource* ViewportResource = Viewport->GetViewportResource();
    if (!ViewportResource)
    {
        return;
    }
    
    PrepareRender(Viewport);

    // Begin Layer Mask
    PrepareLayerPass(Viewport);
    Graphics->DeviceContext->Draw(6, 0);
    CleanUpLayerPass(Viewport);

    // 이 이후로는 LayerInfo가 항상 바인딩 되어있음.
    FRenderTargetRHI* RenderTargetRHI_LayerInfo = ViewportResource->GetRenderTarget(EResourceType::ERT_DepthOfField_LayerInfo);
    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_DepthOfField_LayerInfo), 1, &RenderTargetRHI_LayerInfo->SRV);

    // Begin Max Filter (Near Layer) 
    PrepareMaxFilter_Near(Viewport);
    Graphics->DeviceContext->Draw(6, 0);
    CleanUpMaxFilter_Near(Viewport);

    // Begin CoC Blur (Near Layer) 
    PrepareCoCBlur(Viewport);
    Graphics->DeviceContext->Draw(6, 0);
    CleanUpCoCBlur(Viewport);

    // Begin Near Layer
    PrepareDownSample(Viewport, true);
    Graphics->DeviceContext->Draw(6, 0);
    CleanUpDownSample(Viewport);
    
    PrepareBlur(Viewport, true);
    Graphics->DeviceContext->Draw(6, 0);
    CleanUpBlur(Viewport);

    // Begin Far Layer
    PrepareDownSample(Viewport, false);
    Graphics->DeviceContext->Draw(6, 0);
    CleanUpDownSample(Viewport);

    PrepareBlur(Viewport, false);
    Graphics->DeviceContext->Draw(6, 0);
    CleanUpBlur(Viewport);

    // Begin Composite
    PrepareComposite(Viewport);
    Graphics->DeviceContext->Draw(6, 0);
    CleanUpComposite(Viewport);

    CleanUpRender(Viewport);
}

void FDepthOfFieldRenderPass::PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    ID3D11VertexShader* VertexShader = ShaderManager->GetVertexShaderByKey(L"FullScreenQuadVertexShader");
    Graphics->DeviceContext->VSSetShader(VertexShader, nullptr, 0);
    
    Graphics->DeviceContext->IASetInputLayout(nullptr);
}

void FDepthOfFieldRenderPass::CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    ID3D11ShaderResourceView* NullSRV[1] = { nullptr };
    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_DepthOfField_LayerInfo), 1, NullSRV);
    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_DepthOfField_LayerNear), 1, NullSRV);
    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_DepthOfField_LayerFar), 1, NullSRV);
    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_DepthOfField_FilteredCoC), 1, NullSRV);
    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_DepthOfField_BlurredCoC), 1, NullSRV);
}

void FDepthOfFieldRenderPass::CreateResource()
{
    HRESULT hr = ShaderManager->AddPixelShader(L"GenerateLayer", L"Shaders/DepthOfFieldShader.hlsl", "PS_GenerateLayer");
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to Compile GenerateLayer", L"Error", MB_ICONERROR | MB_OK);
        return;
    }

    D3D_SHADER_MACRO DefinesNear[] =
    {
        { "NEAR", "1" },
        { nullptr, nullptr }
    };
    hr = ShaderManager->AddPixelShader(L"DownSampleNear", L"Shaders/DepthOfFieldShader.hlsl", "PS_ExtractAndDownsampleLayer", DefinesNear);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to Compile DownSampleNear", L"Error", MB_ICONERROR | MB_OK);
        return;
    }

    D3D_SHADER_MACRO DefinesFar[] =
    {
        { "FAR", "1" },
        { nullptr, nullptr }
    };
    hr = ShaderManager->AddPixelShader(L"DownSampleFar", L"Shaders/DepthOfFieldShader.hlsl", "PS_ExtractAndDownsampleLayer", DefinesFar);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to Compile DownSampleFar", L"Error", MB_ICONERROR | MB_OK);
        return;
    }

    hr = ShaderManager->AddPixelShader(L"BlurNearLayer", L"Shaders/DepthOfFieldShader.hlsl", "PS_BlurNearLayer");
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to Compile BlurNearLayer", L"Error", MB_ICONERROR | MB_OK);
        return;
    }

    hr = ShaderManager->AddPixelShader(L"BlurFarLayer", L"Shaders/DepthOfFieldShader.hlsl", "PS_BlurFarLayer");
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to Compile BlurFarLayer", L"Error", MB_ICONERROR | MB_OK);
        return;
    }

    hr = ShaderManager->AddPixelShader(L"Composite", L"Shaders/DepthOfFieldShader.hlsl", "PS_Composite");
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to Compile Composite", L"Error", MB_ICONERROR | MB_OK);
        return;
    }

    hr = ShaderManager->AddPixelShader(L"FilterNearCoC_Max", L"Shaders/DepthOfFieldShader.hlsl", "PS_FilterNearCoC_Max");
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to Compile FilterNearCoC_Max", L"Error", MB_ICONERROR | MB_OK);
        return;
    }

    hr = ShaderManager->AddPixelShader(L"BlurCoCMap", L"Shaders/DepthOfFieldShader.hlsl", "PS_BlurCoCMap");
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to Compile BlurCoCMap", L"Error", MB_ICONERROR | MB_OK);
        return;
    }

    uint32 DepthOfFieldConstantSize = sizeof(FDepthOfFieldConstant);
    hr = BufferManager->CreateBufferGeneric<FDepthOfFieldConstant>("FDepthOfFieldConstant", nullptr, DepthOfFieldConstantSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
}

void FDepthOfFieldRenderPass::PrepareLayerPass(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    FViewportResource* ViewportResource = Viewport->GetViewportResource();
    FRenderTargetRHI* RenderTargetRHI_DepthOfFieldLayer = ViewportResource->GetRenderTarget(EResourceType::ERT_DepthOfField_LayerInfo);
    Graphics->DeviceContext->OMSetRenderTargets(1, &RenderTargetRHI_DepthOfFieldLayer->RTV, nullptr);

    FDepthStencilRHI* DepthStencilRHI_Scene = ViewportResource->GetDepthStencil(EResourceType::ERT_Scene);
    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_SceneDepth), 1, &DepthStencilRHI_Scene->SRV);

    D3D11_TEXTURE2D_DESC TextureDesc;
    DepthStencilRHI_Scene->Texture2D->GetDesc(&TextureDesc);

    D3D11_VIEWPORT Viewport_DepthOfFieldLayer;
    Viewport_DepthOfFieldLayer.Width = static_cast<float>(TextureDesc.Width);
    Viewport_DepthOfFieldLayer.Height = static_cast<float>(TextureDesc.Height);
    Viewport_DepthOfFieldLayer.MinDepth = 0.0f;
    Viewport_DepthOfFieldLayer.MaxDepth = 1.0f;
    Viewport_DepthOfFieldLayer.TopLeftX = 0.f;
    Viewport_DepthOfFieldLayer.TopLeftY = 0.f;
    Graphics->DeviceContext->RSSetViewports(1, &Viewport_DepthOfFieldLayer);

    ID3D11PixelShader* PixelShader = ShaderManager->GetPixelShaderByKey(L"GenerateLayer");
    Graphics->DeviceContext->PSSetShader(PixelShader, nullptr, 0);
    
    FDepthOfFieldConstant DepthOfFieldConstant;
    if (GEngine->ActiveWorld->WorldType == EWorldType::PIE)
    {
        if (const APlayerController* PC = GEngine->ActiveWorld->GetPlayerController())
        {
            if (const APlayerCameraManager* PCM = PC->PlayerCameraManager)
            {
                DepthOfFieldConstant.F_Stop = PCM->F_Stop;
                DepthOfFieldConstant.SensorWidth = PCM->SensorWidth;
                DepthOfFieldConstant.FocalDistance = PCM->FocalDistance;
                DepthOfFieldConstant.FocalLength = PCM->GetFocalLength();
            }
        }
    }
    else
    {
        DepthOfFieldConstant.F_Stop = Viewport->F_Stop;
        DepthOfFieldConstant.SensorWidth = Viewport->SensorWidth;
        DepthOfFieldConstant.FocalDistance = Viewport->FocalDistance;
        DepthOfFieldConstant.FocalLength = Viewport->GetFocalLength();
    }
    BufferManager->UpdateConstantBuffer("FDepthOfFieldConstant", DepthOfFieldConstant);
    BufferManager->BindConstantBuffer("FDepthOfFieldConstant", 1, EShaderStage::Pixel);
}

void FDepthOfFieldRenderPass::CleanUpLayerPass(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);

    ID3D11ShaderResourceView* NullSRV[1] = { nullptr };
    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_SceneDepth), 1, NullSRV);
}

void FDepthOfFieldRenderPass::PrepareMaxFilter_Near(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    FViewportResource* ViewportResource = Viewport->GetViewportResource();
    FRenderTargetRHI* RenderTargetRHI_Target = ViewportResource->GetRenderTarget(EResourceType::ERT_Temp1);
    Graphics->DeviceContext->OMSetRenderTargets(1, &RenderTargetRHI_Target->RTV, nullptr);

    // Full size Viewport

    ID3D11PixelShader* PixelShader = ShaderManager->GetPixelShaderByKey(L"FilterNearCoC_Max");
    Graphics->DeviceContext->PSSetShader(PixelShader, nullptr, 0);

    D3D11_TEXTURE2D_DESC TextureDesc;
    RenderTargetRHI_Target->Texture2D->GetDesc(&TextureDesc);
    FViewportSize ViewportSize;
    ViewportSize.ViewportSize.X = 1.f / static_cast<float>(TextureDesc.Width);
    ViewportSize.ViewportSize.Y = 1.f / static_cast<float>(TextureDesc.Height);
    BufferManager->UpdateConstantBuffer("FViewportSize", ViewportSize);
    BufferManager->BindConstantBuffer("FViewportSize", 0, EShaderStage::Pixel);
}

void FDepthOfFieldRenderPass::CleanUpMaxFilter_Near(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
}

void FDepthOfFieldRenderPass::PrepareCoCBlur(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    FViewportResource* ViewportResource = Viewport->GetViewportResource();
    FRenderTargetRHI* RenderTargetRHI_Target = ViewportResource->GetRenderTarget(EResourceType::ERT_Temp2);
    Graphics->DeviceContext->OMSetRenderTargets(1, &RenderTargetRHI_Target->RTV, nullptr);

    FRenderTargetRHI* RenderTargetRHI_Filtered = ViewportResource->GetRenderTarget(EResourceType::ERT_Temp1);
    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_DepthOfField_FilteredCoC), 1, &RenderTargetRHI_Filtered->SRV);

    D3D11_TEXTURE2D_DESC TextureDesc;
    RenderTargetRHI_Target->Texture2D->GetDesc(&TextureDesc);

    // Full size Viewport

    ID3D11PixelShader* PixelShader = ShaderManager->GetPixelShaderByKey(L"BlurCoCMap");
    Graphics->DeviceContext->PSSetShader(PixelShader, nullptr, 0);

    RenderTargetRHI_Filtered->Texture2D->GetDesc(&TextureDesc);
    FViewportSize ViewportSize;
    ViewportSize.ViewportSize.X = 1.f / static_cast<float>(TextureDesc.Width);
    ViewportSize.ViewportSize.Y = 1.f / static_cast<float>(TextureDesc.Height);
    BufferManager->UpdateConstantBuffer("FViewportSize", ViewportSize);
    BufferManager->BindConstantBuffer("FViewportSize", 0, EShaderStage::Pixel);
}

void FDepthOfFieldRenderPass::CleanUpCoCBlur(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);

    ID3D11ShaderResourceView* NullSRV[1] = { nullptr };
    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_DepthOfField_FilteredCoC), 1, NullSRV);
}

void FDepthOfFieldRenderPass::PrepareDownSample(const std::shared_ptr<FEditorViewportClient>& Viewport, bool bNear)
{
    FViewportResource* ViewportResource = Viewport->GetViewportResource();
    FRenderTargetRHI* RenderTargetRHI_Target = ViewportResource->GetRenderTarget(EResourceType::ERT_Temp1, EDownSampleScale::DSS_2x);
    Graphics->DeviceContext->OMSetRenderTargets(1, &RenderTargetRHI_Target->RTV, nullptr);

    FRenderTargetRHI* RenderTargetRHI_Scene = ViewportResource->GetRenderTarget(EResourceType::ERT_Scene);
    FRenderTargetRHI* RenderTargetRHI_BlurredCoC = ViewportResource->GetRenderTarget(EResourceType::ERT_Temp2);
    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_Scene), 1, &RenderTargetRHI_Scene->SRV);
    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_DepthOfField_BlurredCoC), 1, &RenderTargetRHI_BlurredCoC->SRV);

    D3D11_TEXTURE2D_DESC TextureDesc;
    RenderTargetRHI_Target->Texture2D->GetDesc(&TextureDesc);

    // Down Sampled Viewport
    D3D11_VIEWPORT Viewport_DepthOfFieldLayer;
    Viewport_DepthOfFieldLayer.Width = static_cast<float>(TextureDesc.Width);
    Viewport_DepthOfFieldLayer.Height = static_cast<float>(TextureDesc.Height);
    Viewport_DepthOfFieldLayer.MinDepth = 0.0f;
    Viewport_DepthOfFieldLayer.MaxDepth = 1.0f;
    Viewport_DepthOfFieldLayer.TopLeftX = 0.f;
    Viewport_DepthOfFieldLayer.TopLeftY = 0.f;
    Graphics->DeviceContext->RSSetViewports(1, &Viewport_DepthOfFieldLayer);

    ID3D11PixelShader* PixelShader = ShaderManager->GetPixelShaderByKey(bNear ? L"DownSampleNear" : L"DownSampleFar");
    Graphics->DeviceContext->PSSetShader(PixelShader, nullptr, 0);

    RenderTargetRHI_Scene->Texture2D->GetDesc(&TextureDesc);
    FViewportSize ViewportSize;
    ViewportSize.ViewportSize.X = 1.f / static_cast<float>(TextureDesc.Width);
    ViewportSize.ViewportSize.Y = 1.f / static_cast<float>(TextureDesc.Height);
    BufferManager->UpdateConstantBuffer("FViewportSize", ViewportSize);
    BufferManager->BindConstantBuffer("FViewportSize", 0, EShaderStage::Pixel);
}

void FDepthOfFieldRenderPass::CleanUpDownSample(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);

    ID3D11ShaderResourceView* NullSRV[1] = { nullptr };
    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_Scene), 1, NullSRV);
}

void FDepthOfFieldRenderPass::PrepareBlur(const std::shared_ptr<FEditorViewportClient>& Viewport, bool bNear)
{
    FViewportResource* ViewportResource = Viewport->GetViewportResource();
    const EResourceType RenderTargetType = bNear ? EResourceType::ERT_DepthOfField_LayerNear : EResourceType::ERT_DepthOfField_LayerFar;
    FRenderTargetRHI* RenderTargetRHI_Target = ViewportResource->GetRenderTarget(RenderTargetType, EDownSampleScale::DSS_2x);
    Graphics->DeviceContext->OMSetRenderTargets(1, &RenderTargetRHI_Target->RTV, nullptr);

    FRenderTargetRHI* RenderTargetRHI_LayerDownSample = ViewportResource->GetRenderTarget(EResourceType::ERT_Temp1, EDownSampleScale::DSS_2x);
    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_Scene), 1, &RenderTargetRHI_LayerDownSample->SRV);
    
    // Down Sampled Viewport
    
    ID3D11PixelShader* PixelShader = ShaderManager->GetPixelShaderByKey(bNear ? L"BlurNearLayer" : L"BlurFarLayer");
    Graphics->DeviceContext->PSSetShader(PixelShader, nullptr, 0);

    D3D11_TEXTURE2D_DESC TextureDesc;
    RenderTargetRHI_LayerDownSample->Texture2D->GetDesc(&TextureDesc);
    FViewportSize ViewportSize;
    ViewportSize.ViewportSize.X = 1.f / static_cast<float>(TextureDesc.Width);
    ViewportSize.ViewportSize.Y = 1.f / static_cast<float>(TextureDesc.Height);
    ViewportSize.Padding1 = 1.f;
    BufferManager->UpdateConstantBuffer("FViewportSize", ViewportSize);
    BufferManager->BindConstantBuffer("FViewportSize", 0, EShaderStage::Pixel);
}

void FDepthOfFieldRenderPass::CleanUpBlur(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);

    ID3D11ShaderResourceView* NullSRV[1] = { nullptr };
    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_Scene), 1, NullSRV);
}

void FDepthOfFieldRenderPass::PrepareComposite(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    FViewportResource* ViewportResource = Viewport->GetViewportResource();
    FRenderTargetRHI* RenderTargetRHI_PostProcess = ViewportResource->GetRenderTarget(EResourceType::ERT_DepthOfField_Result);
    Graphics->DeviceContext->OMSetRenderTargets(1, &RenderTargetRHI_PostProcess->RTV, nullptr);
    
    FRenderTargetRHI* RenderTargetRHI_Scene = ViewportResource->GetRenderTarget(EResourceType::ERT_Scene);
    FRenderTargetRHI* RenderTargetRHI_BlurNear = ViewportResource->GetRenderTarget(EResourceType::ERT_DepthOfField_LayerNear, EDownSampleScale::DSS_2x);
    FRenderTargetRHI* RenderTargetRHI_BlurFar = ViewportResource->GetRenderTarget(EResourceType::ERT_DepthOfField_LayerFar, EDownSampleScale::DSS_2x);
    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_Scene), 1, &RenderTargetRHI_Scene->SRV);
    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_DepthOfField_LayerNear), 1, &RenderTargetRHI_BlurNear->SRV);
    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_DepthOfField_LayerFar), 1, &RenderTargetRHI_BlurFar->SRV);
    
    D3D11_TEXTURE2D_DESC TextureDesc;
    RenderTargetRHI_PostProcess->Texture2D->GetDesc(&TextureDesc);
    D3D11_VIEWPORT D3DViewport;
    D3DViewport.Width = Viewport->GetD3DViewport().Width;
    D3DViewport.Height = Viewport->GetD3DViewport().Height;
    D3DViewport.MinDepth = 0.0f;
    D3DViewport.MaxDepth = 1.0f;
    D3DViewport.TopLeftX = 0.f;
    D3DViewport.TopLeftY = 0.f;
    Graphics->DeviceContext->RSSetViewports(1, &D3DViewport);
    
    ID3D11VertexShader* VertexShader = ShaderManager->GetVertexShaderByKey(L"FullScreenQuadVertexShader");
    ID3D11PixelShader* PixelShader = ShaderManager->GetPixelShaderByKey(L"Composite");
    Graphics->DeviceContext->VSSetShader(VertexShader, nullptr, 0);
    Graphics->DeviceContext->PSSetShader(PixelShader, nullptr, 0);
    Graphics->DeviceContext->IASetInputLayout(nullptr);
}

void FDepthOfFieldRenderPass::CleanUpComposite(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);

    ID3D11ShaderResourceView* NullSRV[1] = { nullptr };
    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_Scene), 1, NullSRV);
    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_SceneDepth), 1, NullSRV);
}
