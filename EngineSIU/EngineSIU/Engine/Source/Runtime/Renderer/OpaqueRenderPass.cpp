#include "OpaqueRenderPass.h"

#include <array>

#include "EngineLoop.h"
#include "World/World.h"

#include "RendererHelpers.h"
#include "UnrealClient.h"

#include "UObject/UObjectIterator.h"
#include "UObject/Casts.h"

#include "D3D11RHI/DXDBufferManager.h"
#include "D3D11RHI/GraphicDevice.h"
#include "D3D11RHI/DXDShaderManager.h"

#include "Components/StaticMeshComponent.h"

#include "BaseGizmos/GizmoBaseComponent.h"
#include "Engine/EditorEngine.h"

#include "PropertyEditor/ShowFlags.h"

#include "UnrealEd/EditorViewportClient.h"
#include "Engine/SkeletalMesh.h"

void FOpaqueRenderPass::CreateResource()
{
    // Begin Debug Shaders
    HRESULT hr = ShaderManager->AddPixelShader(L"CommonMeshPixelShaderDepth", L"Shaders/CommonMeshPixelShaderDepth.hlsl", "mainPS");
    if (FAILED(hr))
    {
        return;
    }
    hr = ShaderManager->AddPixelShader(L"CommonMeshPixelShaderWorldNormal", L"Shaders/CommonMeshPixelShaderWorldNormal.hlsl", "mainPS");
    if (FAILED(hr))
    {
        return;
    }
    hr = ShaderManager->AddPixelShader(L"CommonMeshPixelShaderWorldTangent", L"Shaders/CommonMeshPixelShaderWorldTangent.hlsl", "mainPS");
    if (FAILED(hr))
    {
        return;
    }
    // End Debug Shaders

#pragma region UberShader
    D3D_SHADER_MACRO DefinesGouraud[] =
    {
        { GOURAUD, "1" },
        { nullptr, nullptr }
    };
    hr = ShaderManager->AddPixelShader(L"GOURAUD_CommonMeshPixelShader", L"Shaders/CommonMeshPixelShader.hlsl", "mainPS", DefinesGouraud);
    if (FAILED(hr))
    {
        return;
    }
    
    D3D_SHADER_MACRO DefinesLambert[] =
    {
        { LAMBERT, "1" },
        { nullptr, nullptr }
    };
    hr = ShaderManager->AddPixelShader(L"LAMBERT_CommonMeshPixelShader", L"Shaders/CommonMeshPixelShader.hlsl", "mainPS", DefinesLambert);
    if (FAILED(hr))
    {
        return;
    }
    
    D3D_SHADER_MACRO DefinesBlinnPhong[] =
    {
        { PHONG, "1" },
        { nullptr, nullptr }
    };
    hr = ShaderManager->AddPixelShader(L"PHONG_CommonMeshPixelShader", L"Shaders/CommonMeshPixelShader.hlsl", "mainPS", DefinesBlinnPhong);
    if (FAILED(hr))
    {
        return;
    }
    
    D3D_SHADER_MACRO DefinesPBR[] =
    {
        { PBR, "1" },
        { nullptr, nullptr }
    };
    hr = ShaderManager->AddPixelShader(L"PBR_CommonMeshPixelShader", L"Shaders/CommonMeshPixelShader.hlsl", "mainPS", DefinesPBR);
    if (FAILED(hr))
    {
        return;
    }
#pragma endregion UberShader
}

void FOpaqueRenderPass::ChangeViewMode(EViewModeIndex ViewMode)
{
    // Input Layout
    InputLayout_StaticMesh = ShaderManager->GetInputLayoutByKey(L"StaticMeshVertexShader");
    InputLayout_SkeletalMesh = ShaderManager->GetInputLayoutByKey(L"SkeletalMeshVertexShader");

    // Vertex Shader
    if (ViewMode == EViewModeIndex::VMI_Lit_Gouraud)
    {
        VertexShader_StaticMesh = ShaderManager->GetVertexShaderByKey(L"GOURAUD_StaticMeshVertexShader");
        VertexShader_SkeletalMesh = ShaderManager->GetVertexShaderByKey(L"GOURAUD_SkeletalMeshVertexShader");
    }
    else
    {
        VertexShader_StaticMesh = ShaderManager->GetVertexShaderByKey(L"StaticMeshVertexShader");
        VertexShader_SkeletalMesh = ShaderManager->GetVertexShaderByKey(L"SkeletalMeshVertexShader");
    }

    // Pixel Shader
    switch (ViewMode)
    {
    case EViewModeIndex::VMI_Lit_Gouraud:
        PixelShader = ShaderManager->GetPixelShaderByKey(L"GOURAUD_CommonMeshPixelShader");
        break;
    case EViewModeIndex::VMI_Lit_BlinnPhong:
        PixelShader = ShaderManager->GetPixelShaderByKey(L"PHONG_CommonMeshPixelShader");
        break;
    case EViewModeIndex::VMI_LIT_PBR:
        PixelShader = ShaderManager->GetPixelShaderByKey(L"PBR_CommonMeshPixelShader");
        break;
    case EViewModeIndex::VMI_SceneDepth:
        PixelShader = ShaderManager->GetPixelShaderByKey(L"CommonMeshPixelShaderDepth");
        break;
    case EViewModeIndex::VMI_WorldNormal:
        PixelShader = ShaderManager->GetPixelShaderByKey(L"CommonMeshPixelShaderWorldNormal");
        break;
    case EViewModeIndex::VMI_WorldTangent:
        PixelShader = ShaderManager->GetPixelShaderByKey(L"CommonMeshPixelShaderWorldTangent");
        break;
    default:
        PixelShader = ShaderManager->GetPixelShaderByKey(L"LAMBERT_CommonMeshPixelShader");
        break;
    }
    Graphics->DeviceContext->PSSetShader(PixelShader, nullptr, 0);

    UpdateLitUnlitConstant(ViewMode < EViewModeIndex::VMI_Unlit);

    // Rasterizer
    Graphics->ChangeRasterizer(ViewMode);
}

void FOpaqueRenderPass::PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    const EViewModeIndex ViewMode = Viewport->GetViewMode();

    ChangeViewMode(ViewMode);

    Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    TArray<FString> PSBufferKeys = {
        TEXT("FLightInfoBuffer"),
        TEXT("FMaterialConstants"),
        TEXT("FLitUnlitConstants"),
        TEXT("FSubMeshConstants"),
        TEXT("FTextureConstants"),
        TEXT("FIsShadowConstants"),
    };

    BufferManager->BindConstantBuffers(PSBufferKeys, 0, EShaderStage::Pixel);

    BufferManager->BindConstantBuffer(TEXT("FLightInfoBuffer"), 0, EShaderStage::Vertex);
    BufferManager->BindConstantBuffer(TEXT("FMaterialConstants"), 1, EShaderStage::Vertex);
    BufferManager->BindConstantBuffer(TEXT("FCPUSkinningConstants"), 2, EShaderStage::Vertex);
    BufferManager->BindConstantBuffer(TEXT("FObjectConstantBuffer"), 12, EShaderStage::Vertex);
    BufferManager->BindStructuredBufferSRV(TEXT("BoneBuffer"), 1, EShaderStage::Vertex);
    
    Graphics->DeviceContext->RSSetViewports(1, &Viewport->GetViewportResource()->GetD3DViewport());

    const EResourceType ResourceType = EResourceType::ERT_Scene;
    FViewportResource* ViewportResource = Viewport->GetViewportResource();
    FRenderTargetRHI* RenderTargetRHI = ViewportResource->GetRenderTarget(ResourceType);
    FDepthStencilRHI* DepthStencilRHI = ViewportResource->GetDepthStencil(ResourceType);

    Graphics->DeviceContext->OMSetRenderTargets(1, &RenderTargetRHI->RTV, DepthStencilRHI->DSV);

    Graphics->DeviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    Graphics->DeviceContext->OMSetDepthStencilState(Graphics->DepthStencilState_Default, 0);
}

void FOpaqueRenderPass::CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    // 렌더 타겟 해제
    Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    
    ID3D11ShaderResourceView* NullSRV[1] = { nullptr };
    Graphics->DeviceContext->PSSetShaderResources(static_cast<int>(EShaderSRVSlot::SRV_PointLight), 1, NullSRV); // t51 슬롯을 NULL로 설정
    Graphics->DeviceContext->PSSetShaderResources(static_cast<int>(EShaderSRVSlot::SRV_DirectionalLight), 1, NullSRV); // t51 슬롯을 NULL로 설정
    Graphics->DeviceContext->PSSetShaderResources(static_cast<int>(EShaderSRVSlot::SRV_SpotLight), 1, NullSRV); // t51 슬롯을 NULL로 설정

    // 머티리얼 리소스 해제
    constexpr UINT NumViews = static_cast<UINT>(EMaterialTextureSlots::MTS_MAX);
    
    ID3D11ShaderResourceView* NullSRVs[NumViews] = { nullptr };
    
    Graphics->DeviceContext->PSSetShaderResources(0, NumViews, NullSRVs);

    // for Gouraud shading
    ID3D11SamplerState* NullSampler[1] = { nullptr};
    Graphics->DeviceContext->VSSetShaderResources(0, 1, NullSRV);
    Graphics->DeviceContext->VSSetSamplers(0, 1, NullSampler);
    
    // SRV 해제
    ID3D11ShaderResourceView* NullSRVs2[14] = { nullptr };
    Graphics->DeviceContext->PSSetShaderResources(0, 14, NullSRVs2);

    // 상수버퍼 해제
    ID3D11Buffer* NullPSBuffer[8] = { nullptr };
    Graphics->DeviceContext->PSSetConstantBuffers(0, 8, NullPSBuffer);
    ID3D11Buffer* NullVSBuffer[2] = { nullptr };
    Graphics->DeviceContext->VSSetConstantBuffers(0, 2, NullVSBuffer);
}

void FOpaqueRenderPass::PrepareStaticMesh()
{
    Graphics->DeviceContext->VSSetShader(VertexShader_StaticMesh, nullptr, 0);
    Graphics->DeviceContext->IASetInputLayout(InputLayout_StaticMesh);
}

void FOpaqueRenderPass::RenderStaticMesh(const std::shared_ptr<FEditorViewportClient>& Viewport)
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

        UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);

        USceneComponent* SelectedComponent = Engine->GetSelectedComponent();
        AActor* SelectedActor = Engine->GetSelectedActor();

        USceneComponent* TargetComponent = nullptr;

        if (SelectedComponent != nullptr)
        {
            TargetComponent = SelectedComponent;
        }
        else if (SelectedActor != nullptr)
        {
            TargetComponent = SelectedActor->GetRootComponent();
        }

        FMatrix WorldMatrix = Comp->GetWorldMatrix();
        FVector4 UUIDColor = Comp->EncodeUUID() / 255.0f;
        const bool bIsSelected = (Engine && TargetComponent == Comp);

        UpdateObjectConstant(WorldMatrix, UUIDColor, bIsSelected);

        RenderStaticMesh_Internal(RenderData, Comp->GetStaticMesh()->GetMaterials(), Comp->GetOverrideMaterials(), Comp->GetselectedSubMeshIndex());

        if (Viewport->GetShowFlag() & static_cast<uint64>(EEngineShowFlags::SF_AABB))
        {
            FEngineLoop::PrimitiveDrawBatch.AddAABBToBatch(Comp->GetBoundingBox(), Comp->GetComponentLocation(), WorldMatrix);
        }
    }
}

void FOpaqueRenderPass::PrepareSkeletalMesh()
{
    Graphics->DeviceContext->VSSetShader(VertexShader_SkeletalMesh, nullptr, 0);
    Graphics->DeviceContext->IASetInputLayout(InputLayout_SkeletalMesh);
}

void FOpaqueRenderPass::RenderSkeletalMesh(const std::shared_ptr<FEditorViewportClient>& Viewport)
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

        UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);

        USceneComponent* SelectedComponent = Engine->GetSelectedComponent();
        AActor* SelectedActor = Engine->GetSelectedActor();

        USceneComponent* TargetComponent = nullptr;

        if (SelectedComponent != nullptr)
        {
            TargetComponent = SelectedComponent;
        }
        else if (SelectedActor != nullptr)
        {
            TargetComponent = SelectedActor->GetRootComponent();
        }

        FMatrix WorldMatrix = Comp->GetWorldMatrix();
        FVector4 UUIDColor = Comp->EncodeUUID() / 255.0f;
        const bool bIsSelected = (Engine && TargetComponent == Comp);

        UpdateObjectConstant(WorldMatrix, UUIDColor, bIsSelected);

        UpdateBones(Comp);

        RenderSkeletalMesh_Internal(RenderData);

        if (Viewport->GetShowFlag() & static_cast<uint64>(EEngineShowFlags::SF_AABB))
        {
            FEngineLoop::PrimitiveDrawBatch.AddAABBToBatch(Comp->GetBoundingBox(), Comp->GetComponentLocation(), WorldMatrix);
        }
    }
}

void FOpaqueRenderPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    FRenderPassBase::Initialize(InBufferManager, InGraphics, InShaderManager);
}

void FOpaqueRenderPass::PrepareRenderArr()
{
    /**
     * TODO: 현재는 머티리얼이 불투명인지 반투명인지를 구분하지 않고 모두 렌더하고 있음.
     *       제대로 하기 위해선 메시의 머티리얼을 검사하고, 머티리얼을 구분해서 컨테이너에 담아야 함.
     *       스켈레탈 메시의 경우 본 행렬 때문에 스켈레탈 메시 컴포넌트도 참조할 필요 있음.
     */
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

void FOpaqueRenderPass::ClearRenderArr()
{
    StaticMeshComponents.Empty();
    SkeletalMeshComponents.Empty();
}

void FOpaqueRenderPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    PrepareRender(Viewport);

    PrepareStaticMesh();
    RenderStaticMesh(Viewport);

    PrepareSkeletalMesh();
    RenderSkeletalMesh(Viewport);
    
    CleanUpRender(Viewport);
}

void FOpaqueRenderPass::UpdateLitUnlitConstant(int32 IsLit) const
{
    FLitUnlitConstants Data;
    Data.bIsLit = IsLit;
    BufferManager->UpdateConstantBuffer(TEXT("FLitUnlitConstants"), Data);
}
