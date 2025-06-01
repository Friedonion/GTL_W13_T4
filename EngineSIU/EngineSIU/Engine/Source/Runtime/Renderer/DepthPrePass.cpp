#include "DepthPrePass.h"

#include "RendererHelpers.h"
#include "UnrealClient.h"
#include "D3D11RHI/DXDBufferManager.h"
#include "D3D11RHI/GraphicDevice.h"
#include "D3D11RHI/DXDShaderManager.h"
#include "UnrealEd/EditorViewportClient.h"
#include "UObject/UObjectIterator.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "InteractiveToolsFramework/BaseGizmos/GizmoBaseComponent.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"

void FDepthPrePass::PrepareRenderArr()
{
    for (const auto Iter : TObjectRange<UMeshComponent>())
    {
        if (Iter->GetWorld() != GEngine->ActiveWorld)
        {
            continue;
        }

        if (USkeletalMeshComponent* SkeletalMeshComp = Cast<USkeletalMeshComponent>(Iter))
        {
            SkeletalMeshComponents.Add(SkeletalMeshComp);
        }
        else if (UStaticMeshComponent* StaticMeshComp = Cast<UStaticMeshComponent>(Iter))
        {
            if (Iter->IsA<UGizmoBaseComponent>())
            {
                continue;
            }

            StaticMeshComponents.Add(StaticMeshComp);       
        }
    }
}

void FDepthPrePass::ClearRenderArr()
{
    StaticMeshComponents.Empty();
    SkeletalMeshComponents.Empty();
}

void FDepthPrePass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    PrepareRender(Viewport);

    PrepareStaticMesh();
    RenderStaticMesh();

    PrepareSkeletalMesh();
    RenderSkeletalMesh();
    
    CleanUpRender(Viewport);
}

void FDepthPrePass::PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    // 뎁스만 필요하므로, 픽셀 쉐이더는 지정 안함.
    Graphics->DeviceContext->PSSetShader(nullptr, nullptr, 0);

    Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    Graphics->DeviceContext->RSSetState(Graphics->RasterizerSolidBack);

    Graphics->DeviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);

    Graphics->DeviceContext->OMSetDepthStencilState(Graphics->DepthStencilState_Default, 1);

    FViewportResource* ViewportResource = Viewport->GetViewportResource();
    FDepthStencilRHI* DepthStencilRHI = ViewportResource->GetDepthStencil(EResourceType::ERT_Debug);

    Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, DepthStencilRHI->DSV); // ← 깊이 전용

    BufferManager->BindStructuredBufferSRV(TEXT("BoneBuffer"), 1, EShaderStage::Vertex);
    BufferManager->BindConstantBuffer(TEXT("FCPUSkinningConstants"), 2, EShaderStage::Vertex);

    FCPUSkinningConstants CPUSkinningData;
    CPUSkinningData.bCPUSkinning = USkeletalMeshComponent::GetCPUSkinning();
    BufferManager->UpdateConstantBuffer(TEXT("FCPUSkinningConstants"), CPUSkinningData);
}

void FDepthPrePass::CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    // 렌더 타겟 해제
    Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);

    ID3D11ShaderResourceView* NullSRV[1] = { nullptr };
    Graphics->DeviceContext->VSSetShaderResources(1, 1, NullSRV);
}

void FDepthPrePass::PrepareStaticMesh()
{
    ID3D11VertexShader* VertexShader = ShaderManager->GetVertexShaderByKey(L"StaticMeshVertexShader");
    ID3D11InputLayout* InputLayout = ShaderManager->GetInputLayoutByKey(L"StaticMeshVertexShader");
    
    Graphics->DeviceContext->VSSetShader(VertexShader, nullptr, 0);
    Graphics->DeviceContext->IASetInputLayout(InputLayout);
}

void FDepthPrePass::PrepareSkeletalMesh()
{
    ID3D11VertexShader* VertexShader = ShaderManager->GetVertexShaderByKey(L"SkeletalMeshVertexShader");
    ID3D11InputLayout* InputLayout = ShaderManager->GetInputLayoutByKey(L"SkeletalMeshVertexShader");
    
    Graphics->DeviceContext->VSSetShader(VertexShader, nullptr, 0);
    Graphics->DeviceContext->IASetInputLayout(InputLayout);
}

void FDepthPrePass::RenderStaticMesh()
{
    for (UStaticMeshComponent* Comp : StaticMeshComponents)
    {
        if (!Comp || !Comp->GetStaticMesh())
        {
            continue;
        }

        FStaticMeshRenderData* RenderData = Comp->GetStaticMesh()->GetRenderData();
        if (RenderData == nullptr)
        {
            continue;
        }

        FMatrix WorldMatrix = Comp->GetWorldMatrix();
        FVector4 UUIDColor = Comp->EncodeUUID() / 255.0f;
        constexpr bool bIsSelected = false;

        UpdateObjectConstant(WorldMatrix, UUIDColor, bIsSelected);

        RenderStaticMesh_Internal(RenderData, Comp->GetStaticMesh()->GetMaterials(), Comp->GetOverrideMaterials(), Comp->GetselectedSubMeshIndex());
    }
}

void FDepthPrePass::RenderSkeletalMesh()
{
    for (const USkeletalMeshComponent* Comp : SkeletalMeshComponents)
    {
        if (!Comp || !Comp->GetSkeletalMeshAsset())
        {
            continue;
        }
        const FSkeletalMeshRenderData* RenderData = Comp->GetCPUSkinning() ? Comp->GetCPURenderData() : Comp->GetSkeletalMeshAsset()->GetRenderData();
        if (RenderData == nullptr)
        {
            continue;
        }

        FMatrix WorldMatrix = Comp->GetWorldMatrix();
        FVector4 UUIDColor = Comp->EncodeUUID() / 255.0f;
        constexpr bool bIsSelected = false;

        UpdateObjectConstant(WorldMatrix, UUIDColor, bIsSelected);

        UpdateBones(Comp);

        RenderSkeletalMesh_Internal(RenderData);
    }
}
