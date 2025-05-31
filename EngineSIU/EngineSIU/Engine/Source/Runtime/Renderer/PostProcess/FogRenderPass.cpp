#include "FogRenderPass.h"
#include "D3D11RHI/GraphicDevice.h"
#include "D3D11RHI/DXDShaderManager.h"
#include "D3D11RHI/DXDBufferManager.h"
#include "UnrealEd/EditorViewportClient.h"
#include "Define.h"
#include "Engine/Classes/GameFramework/Actor.h"
#include <wchar.h>
#include <UObject/UObjectIterator.h>
#include <Engine/Engine.h>

#include "RendererHelpers.h"
#include "UnrealClient.h"
#include "PropertyEditor/ShowFlags.h"

void FFogRenderPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    FRenderPassBase::Initialize(InBufferManager, InGraphics, InShaderManager);
}

void FFogRenderPass::CreateResource()
{
    // 정점 셰이더 및 입력 레이아웃 생성
    HRESULT hr = ShaderManager->AddVertexShader(L"FogVertexShader", L"Shaders/FogShader.hlsl", "mainVS");
    if (FAILED(hr))
    {
        return;
    }
    // 픽셀 셰이더 생성
    hr = ShaderManager->AddPixelShader(L"FogPixelShader", L"Shaders/FogShader.hlsl", "mainPS");
    if (FAILED(hr))
    {
        return;
    }
    
    // 생성된 셰이더와 입력 레이아웃 획득
    VertexShader = ShaderManager->GetVertexShaderByKey(L"FogVertexShader");
    PixelShader = ShaderManager->GetPixelShaderByKey(L"FogPixelShader");
}

void FFogRenderPass::UpdateShader()
{
    VertexShader = ShaderManager->GetVertexShaderByKey(L"FogVertexShader");
    PixelShader = ShaderManager->GetPixelShaderByKey(L"FogPixelShader");
}

void FFogRenderPass::PrepareRenderArr()
{
    for (const auto Iter : TObjectRange<UHeightFogComponent>())
    {
        if (Iter->GetWorld() == GEngine->ActiveWorld)
        {
            FogComponents.Add(Iter);
        }
    }
}

void FFogRenderPass::ClearRenderArr()
{
    FogComponents.Empty();
}

void FFogRenderPass::PrepareRenderState()
{
    // 셰이더 설정
    Graphics->DeviceContext->VSSetShader(VertexShader, nullptr, 0);
    Graphics->DeviceContext->PSSetShader(PixelShader, nullptr, 0);

    Graphics->DeviceContext->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
    Graphics->DeviceContext->IASetInputLayout(nullptr);
    Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Graphics->DeviceContext->RSSetState(Graphics->RasterizerSolidBack);

    TArray<FString> PSBufferKeys = {
        TEXT("FFogConstants")
    };

    BufferManager->BindConstantBuffers(PSBufferKeys, 0, EShaderStage::Pixel);
}

void FFogRenderPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    const uint64 ShowFlag = Viewport->GetShowFlag();
    const EViewModeIndex ViewMode = Viewport->GetViewMode();
    
    if (ViewMode == EViewModeIndex::VMI_Wireframe || FogComponents.Num() <= 0 || !(ShowFlag & static_cast<uint64>(EEngineShowFlags::SF_Fog)))
    {
        return;
    }

    FViewportResource* ViewportResource = Viewport->GetViewportResource();
    const EResourceType ResourceType = EResourceType::ERT_Scene; 
    FRenderTargetRHI* RenderTargetRHI = ViewportResource->GetRenderTarget(ResourceType);

    Graphics->DeviceContext->OMSetRenderTargets(1, &RenderTargetRHI->RTV, nullptr);
    Graphics->DeviceContext->OMSetBlendState(Graphics->BlendState_AlphaBlend, nullptr, 0xffffffff);

    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_SceneDepth), 1, &ViewportResource->GetDepthStencil(EResourceType::ERT_Scene)->SRV);
    
    UpdateShader();

    PrepareRenderState();
    
    for (const auto& Fog : FogComponents)
    {
        if (Fog->GetFogDensity() > 0)
        {
            UpdateFogConstant(Fog);

            Graphics->DeviceContext->Draw(6, 0);
        }
    }

    ID3D11ShaderResourceView* NullSRV[1] = { nullptr };
    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_SceneDepth), 1, NullSRV);

    Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    Graphics->DeviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
}

void FFogRenderPass::UpdateFogConstant(UHeightFogComponent* Fog)
{
    FFogConstants Constants; 
    {
        Constants.FogColor = Fog->GetFogColor();
        Constants.FogDensity = Fog->GetFogDensity();
        Constants.FogDistanceWeight = Fog->GetFogDistanceWeight();
        Constants.FogHeightFalloff = Fog->GetFogHeightFalloff();
        Constants.StartDistance = Fog->GetStartDistance();
        Constants.EndDistance = Fog->GetEndDistance();
        Constants.FogHeight = Fog->GetComponentLocation().Z;
    }
    //상수버퍼 업데이트
    BufferManager->UpdateConstantBuffer(TEXT("FFogConstants"), Constants);
}

void FFogRenderPass::PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
}

void FFogRenderPass::CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
}
