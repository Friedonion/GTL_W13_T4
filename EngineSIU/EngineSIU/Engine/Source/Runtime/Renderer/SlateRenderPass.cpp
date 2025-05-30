#include "SlateRenderPass.h"

#include "RendererHelpers.h"
#include "UnrealClient.h"
#include "D3D11RHI/DXDBufferManager.h"
#include "D3D11RHI/DXDShaderManager.h"
#include "Engine/Engine.h"
#include "UnrealEd/EditorViewportClient.h"
#include "UObject/Object.h"
#include "World/World.h"

void FSlateRenderPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    FRenderPassBase::Initialize(InBufferManager, InGraphics, InShaderManager);
}

void FSlateRenderPass::PrepareRenderArr()
{
}

void FSlateRenderPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    const FRect Rect = Viewport->GetViewport()->GetRect();

    uint32 ClientWidth = 0;
    uint32 ClientHeight = 0;
    GEngineLoop.GetClientSize(ClientWidth, ClientHeight);

    float ClientWidthFloat = static_cast<float>(ClientWidth);
    float ClientHeightFloat = static_cast<float>(ClientHeight);

    // 버퍼 업데이트
    FSlateTransform Transform;
    // Client에서의 FViewport 위치를 기반으로 Scale과 Offset 계산

    
    if (GEngine->ActiveWorld->WorldType == EWorldType::ParticleViewer)
    {
        ClientWidthFloat /= 0.5f;
        ClientHeightFloat /= 0.5f;
        
        Transform.Scale = FVector2D(
    Rect.Width / ClientWidthFloat,
    Rect.Height / ClientHeightFloat
        );
        
        Transform.Offset = FVector2D(
    (Rect.TopLeftX + Rect.Width * 0.5f) / ClientWidthFloat * 2.0f - 1.0f,
    1.0f - (Rect.TopLeftY + Rect.Height * 0.5f) / ClientHeightFloat * 2.0f
        );
    }
    else if (GEngine->ActiveWorld->WorldType == EWorldType::PhysicsAssetViewer)
    {
        ClientWidthFloat /= 0.75f;
        ClientHeightFloat /= 1.0f;

        Transform.Scale = FVector2D(
            Rect.Width / ClientWidthFloat,
            Rect.Height / ClientHeightFloat
        );

        float centerX = Rect.TopLeftX + Rect.Width * 0.5f; // 중앙 정렬용
        float ndcX = centerX / ClientWidthFloat * 2.0f - 0.6f;

        float centerY = Rect.TopLeftY + Rect.Height * 0.5f;
        float ndcY = 1.0f - centerY / ClientHeightFloat * 2.0f;

        Transform.Offset = FVector2D(
            ndcX,
            ndcY
        );
    }
    else
    {
        Transform.Scale = FVector2D(
            Rect.Width / ClientWidthFloat,
            Rect.Height / ClientHeightFloat
        );
        Transform.Offset = FVector2D(
            (Rect.TopLeftX + Rect.Width * 0.5f) / ClientWidthFloat * 2.0f - 1.0f,
            1.0f - (Rect.TopLeftY + Rect.Height * 0.5f) / ClientHeightFloat * 2.0f
        );
    }

    // SlateTransform 버퍼 업데이트
    BufferManager->UpdateConstantBuffer<FSlateTransform>("FSlateTransform", Transform);
    BufferManager->BindConstantBuffer(TEXT("FSlateTransform"), 11, EShaderStage::Vertex);

    // 렌더 타겟을 백버퍼로 지정
    Graphics->DeviceContext->OMSetRenderTargets(1, &Graphics->BackBufferRTV, nullptr);
    Graphics->DeviceContext->RSSetViewports(1, &Graphics->Viewport);

    // 렌더 준비
    FViewportResource* ViewportResource = Viewport->GetViewportResource();
    FRenderTargetRHI* Resource = ViewportResource->GetRenderTarget(EResourceType::ERT_Compositing);

    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_Viewport), 1, &Resource->SRV);
    Graphics->DeviceContext->PSSetSamplers(0, 1, &Sampler);

    ID3D11VertexShader* VertexShader = ShaderManager->GetVertexShaderByKey(L"SlateShader");
    ID3D11PixelShader* PixelShader = ShaderManager->GetPixelShaderByKey(L"SlateShader");
    Graphics->DeviceContext->VSSetShader(VertexShader, nullptr, 0);
    Graphics->DeviceContext->PSSetShader(PixelShader, nullptr, 0);
    Graphics->DeviceContext->IASetInputLayout(nullptr);

    // Quad 렌더
    Graphics->DeviceContext->Draw(6, 0);

    // Clear: 사용한 리소스 해제
    ID3D11ShaderResourceView* NullSRV[1] = { nullptr };
    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_Viewport), 1, NullSRV);
}

void FSlateRenderPass::ClearRenderArr()
{
}

void FSlateRenderPass::CreateResource()
{
    HRESULT Result = ShaderManager->AddVertexShader(L"SlateShader", L"Shaders/SlateShader.hlsl", "mainVS");
    if (FAILED(Result))
    {
        return;
    }
    
    Result = ShaderManager->AddPixelShader(L"SlateShader", L"Shaders/SlateShader.hlsl", "mainPS");
    if (FAILED(Result))
    {
        return;
    }
    
    CreateBuffer();
    CreateSampler();
}

void FSlateRenderPass::CreateBuffer()
{
    HRESULT Result = BufferManager->CreateBufferGeneric<FSlateTransform>(
        "FSlateTransform", nullptr, sizeof(FSlateTransform), D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC,
        D3D11_CPU_ACCESS_WRITE
    );
    if (FAILED(Result))
    {
        return;
    }
}

void FSlateRenderPass::CreateSampler()
{
    D3D11_SAMPLER_DESC SamplerDesc = {};
    SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    SamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    SamplerDesc.MinLOD = 0;
    SamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    
    Graphics->Device->CreateSamplerState(&SamplerDesc, &Sampler);
}

void FSlateRenderPass::PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
}

void FSlateRenderPass::CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
}

