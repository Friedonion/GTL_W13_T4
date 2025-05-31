#include "BillboardRenderPass.h"

#include "D3D11RHI/DXDBufferManager.h"
#include "D3D11RHI/GraphicDevice.h"
#include "D3D11RHI/DXDShaderManager.h"

#include "UObject/UObjectIterator.h"
#include "UObject/Casts.h"

#include "UnrealEd/EditorViewportClient.h"

#include "Components/BillboardComponent.h"
#include "Components/ParticleSubUVComponent.h"
#include "Components/TextComponent.h"
#include "Engine/EditorEngine.h"

#include "EngineLoop.h"
#include "UnrealClient.h"
#include "LevelEditor/SLevelEditor.h"

#include "World/World.h"

FBillboardRenderPass::FBillboardRenderPass()
    : ResourceType(EResourceType::ERT_Scene)
{
}

void FBillboardRenderPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    FRenderPassBase::Initialize(InBufferManager, InGraphics, InShaderManager);
}

void FBillboardRenderPass::PrepareRenderArr()
{
    BillboardComps.Empty();
    for (const auto Iter : TObjectRange<UBillboardComponent>())
    {
        if (Iter->GetWorld() == GEngine->ActiveWorld)
        {
            BillboardComps.Add(Iter);
        }
    }

    BillboardComps.Sort(
        [](const UBillboardComponent* A, const UBillboardComponent* B)
        {
            const FVector LocA = A->GetComponentLocation();
            const FVector LocB = B->GetComponentLocation();
            const FVector LocCam = GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->GetCameraLocation();

            const float DistA = (LocCam - LocA).SquaredLength();
            const float DistB = (LocCam - LocB).SquaredLength();
            
            return DistA > DistB;
        }
    );
}

void FBillboardRenderPass::UpdateSubUVConstant(FVector2D UVOffset, FVector2D UVScale) const
{
    FSubUVConstant Data;
    Data.uvOffset = UVOffset;
    Data.uvScale = UVScale;

    BufferManager->UpdateConstantBuffer(TEXT("FSubUVConstant"), Data);
}

void FBillboardRenderPass::RenderTexturePrimitive(ID3D11Buffer* pVertexBuffer, UINT NumVertices, ID3D11Buffer* pIndexBuffer, UINT NumIndices, ID3D11ShaderResourceView* TextureSRV, ID3D11SamplerState* SamplerState) const
{
    SetupVertexBuffer(pVertexBuffer, NumVertices);

    Graphics->DeviceContext->IASetIndexBuffer(pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    Graphics->DeviceContext->PSSetShaderResources(0, 1, &TextureSRV);
    Graphics->DeviceContext->DrawIndexed(NumIndices, 0, 0);
}

void FBillboardRenderPass::RenderTextPrimitive(ID3D11Buffer* pVertexBuffer, UINT NumVertices, ID3D11ShaderResourceView* TextureSRV, ID3D11SamplerState* SamplerState) const
{
    SetupVertexBuffer(pVertexBuffer, NumVertices);

    Graphics->DeviceContext->PSSetShaderResources(0, 1, &TextureSRV);
    Graphics->DeviceContext->Draw(NumVertices, 0);
}

void FBillboardRenderPass::CreateResource()
{
    // Billboard 셰이더 생성
    D3D11_INPUT_ELEMENT_DESC TextureLayoutDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    HRESULT hr = ShaderManager->AddVertexShaderAndInputLayout(L"BillBoardVertexShader", L"Shaders/BillBoardVertexShader.hlsl", "main", TextureLayoutDesc, ARRAYSIZE(TextureLayoutDesc));
    if (FAILED(hr))
    {
        return;
    }
    
    hr = ShaderManager->AddPixelShader(L"BillBoardPixelShader", L"Shaders/BillBoardPixelShader.hlsl", "main");
    if (FAILED(hr))
    {
        return;
    }
}

void FBillboardRenderPass::UpdateShader()
{
    ID3D11VertexShader* VertexShader = ShaderManager->GetVertexShaderByKey(L"BillBoardVertexShader");
    ID3D11InputLayout* InputLayout = ShaderManager->GetInputLayoutByKey(L"BillBoardVertexShader");
    ID3D11PixelShader* PixelShader = ShaderManager->GetPixelShaderByKey(L"BillBoardPixelShader");
    
    Graphics->DeviceContext->VSSetShader(VertexShader, nullptr, 0);
    Graphics->DeviceContext->PSSetShader(PixelShader, nullptr, 0);
    Graphics->DeviceContext->IASetInputLayout(InputLayout);
}

void FBillboardRenderPass::PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    FViewportResource* ViewportResource = Viewport->GetViewportResource();

    const FRenderTargetRHI* RenderTargetRHI = ViewportResource->GetRenderTarget(ResourceType);
    const FDepthStencilRHI* DepthStencilRHI = ViewportResource->GetDepthStencil(EResourceType::ERT_Scene);

    Graphics->DeviceContext->OMSetRenderTargets(1, &RenderTargetRHI->RTV, DepthStencilRHI->DSV);

    Graphics->DeviceContext->RSSetViewports(1, &Viewport->GetViewportResource()->GetD3DViewport());

    Graphics->DeviceContext->RSSetState(Graphics->RasterizerSolidBack);
    
    Graphics->DeviceContext->OMSetBlendState(Graphics->BlendState_AlphaBlend, nullptr, 0xffffffff);
    Graphics->DeviceContext->OMSetDepthStencilState(Graphics->DepthStencilState_DepthWriteDisabled, 1);
    
    Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    UpdateShader();

    BufferManager->BindConstantBuffer(TEXT("FObjectConstantBuffer"), 0, EShaderStage::Vertex);

    BufferManager->BindConstantBuffer(TEXT("FSubUVConstant"), 1, EShaderStage::Pixel);
}

void FBillboardRenderPass::CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);

    Graphics->DeviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    Graphics->DeviceContext->OMSetDepthStencilState(Graphics->DepthStencilState_Default, 1);
}

void FBillboardRenderPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    PrepareRender(Viewport);

    FVertexInfo VertexInfo;
    FIndexInfo IndexInfo;

    BufferManager->GetQuadBuffer(VertexInfo, IndexInfo);

    // 각 Billboard에 대해 렌더링 처리
    for (auto BillboardComp : BillboardComps)
    {
        UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);
        
        FMatrix Model = BillboardComp->CreateBillboardMatrix();
        FVector4 UUIDColor = BillboardComp->EncodeUUID() / 255.0f;
        bool bIsSelected = (Engine && Engine->GetSelectedActor() == BillboardComp->GetOwner());
        UpdateObjectConstant(Model, UUIDColor, bIsSelected);

        if (UParticleSubUVComponent* SubUVParticle = Cast<UParticleSubUVComponent>(BillboardComp))
        {
            UpdateSubUVConstant(SubUVParticle->GetUVOffset(), SubUVParticle->GetUVScale());

            RenderTexturePrimitive(
                VertexInfo.VertexBuffer,
                VertexInfo.NumVertices,
                IndexInfo.IndexBuffer,
                IndexInfo.NumIndices,
                SubUVParticle->Texture->TextureSRV,
                Graphics->GetSamplerState(SubUVParticle->Texture->SamplerType)
            );
        }
        else if (UTextComponent* TextComp = Cast<UTextComponent>(BillboardComp))
        {
            FBufferInfo Buffers;
            float Height = TextComp->Texture->Height;
            float Width = TextComp->Texture->Width;
            BufferManager->CreateUnicodeTextBuffer(TextComp->GetText(), Buffers, Width, Height, TextComp->GetColumnCount(), TextComp->GetRowCount());

            UpdateSubUVConstant(FVector2D(), FVector2D(1, 1));

            RenderTextPrimitive(
                Buffers.VertexInfo.VertexBuffer,
                Buffers.VertexInfo.NumVertices,
                TextComp->Texture->TextureSRV,
                Graphics->GetSamplerState(TextComp->Texture->SamplerType)
            );
        }
        else
        {
            UpdateSubUVConstant(FVector2D(BillboardComp->FinalIndexU, BillboardComp->FinalIndexV), FVector2D(1, 1));

            RenderTexturePrimitive(
                VertexInfo.VertexBuffer,
                VertexInfo.NumVertices,
                IndexInfo.IndexBuffer,
                IndexInfo.NumIndices,
                BillboardComp->Texture->TextureSRV,
                Graphics->GetSamplerState(BillboardComp->Texture->SamplerType)
            );
        }
    }

    CleanUpRender(Viewport);
}

void FBillboardRenderPass::SetupVertexBuffer(ID3D11Buffer* pVertexBuffer, UINT NumVertices) const
{
    UINT Stride = sizeof(FVertexTexture);
    UINT Offset = 0;
    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &Stride, &Offset);
}

void FBillboardRenderPass::ClearRenderArr()
{
    BillboardComps.Empty();
}
