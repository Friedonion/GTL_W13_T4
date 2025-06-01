
#include "GizmoRenderPass.h"

#include <array>

#include "UObject/UObjectIterator.h"
#include "UObject/Casts.h"

#include "D3D11RHI/DXDBufferManager.h"
#include "D3D11RHI/GraphicDevice.h"
#include "D3D11RHI/DXDShaderManager.h"

#include "RendererHelpers.h"
#include "Math/JungleMath.h"

#include "Actors/Player.h"

#include "BaseGizmos/GizmoBaseComponent.h"

#include "UnrealEd/EditorViewportClient.h"

#include "EngineLoop.h"
#include "UnrealClient.h"
#include "BaseGizmos/TransformGizmo.h"

#include "UObject/ObjectTypes.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/EditorEngine.h"
#include "LevelEditor/SLevelEditor.h"

void FGizmoRenderPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    FRenderPassBase::Initialize(InBufferManager, InGraphics, InShaderManager);
}

void FGizmoRenderPass::CreateResource()
{
    HRESULT hr = ShaderManager->AddPixelShader(L"GizmoPixelShader", L"Shaders/GizmoPixelShader.hlsl", "mainPS");
    if (FAILED(hr))
    {
        return;
    }
}

void FGizmoRenderPass::UpdateShader()
{
    ID3D11VertexShader* VertexShader = ShaderManager->GetVertexShaderByKey(L"StaticMeshVertexShader");
    ID3D11InputLayout* InputLayout = ShaderManager->GetInputLayoutByKey(L"StaticMeshVertexShader");
    ID3D11PixelShader* PixelShader = ShaderManager->GetPixelShaderByKey(L"GizmoPixelShader");

    Graphics->DeviceContext->VSSetShader(VertexShader, nullptr, 0);
    Graphics->DeviceContext->PSSetShader(PixelShader, nullptr, 0);
    Graphics->DeviceContext->IASetInputLayout(InputLayout);
}

void FGizmoRenderPass::PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    FViewportResource* ViewportResource = Viewport->GetViewportResource();
    FRenderTargetRHI* RenderTargetRHI = ViewportResource->GetRenderTarget(EResourceType::ERT_EditorOverlay);
    FDepthStencilRHI* DepthStencilRHI = ViewportResource->GetDepthStencil(EResourceType::ERT_Gizmo);
    
    Graphics->DeviceContext->OMSetRenderTargets(1, &RenderTargetRHI->RTV, DepthStencilRHI->DSV);

    // 씬 뎁스를 쉐이더 리소스로 사용
    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_SceneDepth), 1, &ViewportResource->GetDepthStencil(EResourceType::ERT_Scene)->SRV);
    
    Graphics->DeviceContext->RSSetState(FEngineLoop::GraphicDevice.RasterizerSolidBack);

    FViewportSize ViewportSize;
    ViewportSize.ViewportSize.X = Viewport->GetViewport()->GetRect().Width;
    ViewportSize.ViewportSize.Y = Viewport->GetViewport()->GetRect().Height;
    BufferManager->UpdateConstantBuffer(TEXT("FViewportSize"), ViewportSize);

    UpdateShader();

    Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    BufferManager->BindConstantBuffer(TEXT("FMaterialConstants"), 1, EShaderStage::Pixel);
    BufferManager->BindConstantBuffer(TEXT("FViewportSize"), 2, EShaderStage::Pixel);
}

void FGizmoRenderPass::CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    Graphics->DeviceContext->RSSetState(Graphics->GetCurrentRasterizer());

    Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    ID3D11ShaderResourceView* NullSRV[1] = { nullptr };
    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_SceneDepth), 1, NullSRV);
}

void FGizmoRenderPass::ClearRenderArr()
{
    GizmoComponents.Empty();
}

void FGizmoRenderPass::PrepareRenderArr()
{
    UEditorEngine* EditorEngine = Cast<UEditorEngine>(GEngine);
    if (!ShouldRenderGizmo(EditorEngine))
    {
        return;
    }

    std::shared_ptr<FEditorViewportClient> Viewport = GEngineLoop.GetLevelEditor()->GetActiveViewportClient();
    EControlMode Mode = EditorEngine->GetEditorPlayer()->GetControlMode();
    if (Mode == CM_TRANSLATION)
    {
        for (UStaticMeshComponent* StaticMeshComp : Viewport->GetGizmoActor()->GetArrowArr())
        {
            if (UGizmoBaseComponent* GizmoComp = Cast<UGizmoBaseComponent>(StaticMeshComp))
            {
                GizmoComponents.Add(GizmoComp);
            }
        }
    }
    else if (Mode == CM_SCALE)
    {
        for (UStaticMeshComponent* StaticMeshComp : Viewport->GetGizmoActor()->GetScaleArr())
        {
            if (UGizmoBaseComponent* GizmoComp = Cast<UGizmoBaseComponent>(StaticMeshComp))
            {
                GizmoComponents.Add(GizmoComp);
            }
        }
    }
    else if (Mode == CM_ROTATION)
    {
        for (UStaticMeshComponent* StaticMeshComp : Viewport->GetGizmoActor()->GetDiscArr())
        {
            if (UGizmoBaseComponent* GizmoComp = Cast<UGizmoBaseComponent>(StaticMeshComp))
            {
                GizmoComponents.Add(GizmoComp);
            }
        }
    }
}

void FGizmoRenderPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    UEditorEngine* EditorEngine = Cast<UEditorEngine>(GEngine);
    if (!ShouldRenderGizmo(EditorEngine))
    {
        return;
    }
    
    PrepareRender(Viewport);
    
    for (const UGizmoBaseComponent* GizmoComp : GizmoComponents)
    {
        // 오브젝트 버퍼 업데이트
        FMatrix WorldMatrix = GizmoComp->GetWorldMatrix();
        FVector4 UUIDColor = GizmoComp->EncodeUUID() / 255.0f;
        bool bIsSelected = (GizmoComp == Viewport->GetPickedGizmoComponent());
        UpdateObjectConstant(WorldMatrix, UUIDColor, bIsSelected);
        
        FStaticMeshRenderData* RenderData = GizmoComp->GetStaticMesh()->GetRenderData();
        RenderStaticMesh_Internal(
            RenderData,
            GizmoComp->GetStaticMesh()->GetMaterials(),
            GizmoComp->GetOverrideMaterials(),
            GizmoComp->GetselectedSubMeshIndex()
        );
    }
    
    CleanUpRender(Viewport);
}

bool FGizmoRenderPass::ShouldRenderGizmo(const UEditorEngine* EditorEngine) const
{
    return EditorEngine && (EditorEngine->GetSelectedActor() || EditorEngine->GetSelectedComponent());
}
