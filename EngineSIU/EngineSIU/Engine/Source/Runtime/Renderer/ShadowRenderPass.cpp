#include "ShadowRenderPass.h"

#include "ShadowManager.h"
#include "BaseGizmos/GizmoBaseComponent.h"
#include "Components/Light/LightComponent.h"
#include "Components/Light/PointLightComponent.h"
#include "D3D11RHI/DXDBufferManager.h"
#include "D3D11RHI/GraphicDevice.h"
#include "D3D11RHI/DXDShaderManager.h"
#include "Components/Light/DirectionalLightComponent.h"
#include "Components/Light/PointLightComponent.h"
#include "Components/Light/SpotLightComponent.h"
#include "Engine/EditorEngine.h"
#include "Engine/Engine.h"
#include "UnrealEd/EditorViewportClient.h"
#include "UObject/Casts.h"
#include "UObject/UObjectIterator.h"
#include "Editor/PropertyEditor/ShowFlags.h"
#include "Engine/AssetManager.h"

class UEditorEngine;
class UStaticMeshComponent;
#include "UnrealEd/EditorViewportClient.h"

void FShadowRenderPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    FRenderPassBase::Initialize(InBufferManager, InGraphics, InShaderManager);
}

void FShadowRenderPass::InitializeShadowManager(class FShadowManager* InShadowManager)
{
    ShadowManager = InShadowManager;
}


//한번만 실행하면 되는 것
void FShadowRenderPass::PrepareRenderState()
{
    // Shader Hot Reload 대응 
    StaticMeshIL = ShaderManager->GetInputLayoutByKey(L"StaticMeshVertexShader");
    DepthOnlyVS = ShaderManager->GetVertexShaderByKey(L"DepthOnlyVS");
    DepthOnlyPS = ShaderManager->GetPixelShaderByKey(L"DepthOnlyPS");
    
    Graphics->DeviceContext->IASetInputLayout(StaticMeshIL);
    Graphics->DeviceContext->VSSetShader(DepthOnlyVS, nullptr, 0);

    // Note : PS만 언바인드할 뿐, UpdateLightBuffer에서 바인딩된 SRV 슬롯들은 그대로 남아 있음
    Graphics->DeviceContext->PSSetShader(nullptr, nullptr, 0);
    Graphics->DeviceContext->RSSetState(Graphics->RasterizerShadow);
    
    BufferManager->BindConstantBuffer(TEXT("FShadowConstantBuffer"), 11, EShaderStage::Vertex);
    BufferManager->BindConstantBuffer(TEXT("FShadowConstantBuffer"), 11, EShaderStage::Pixel);
    BufferManager->BindConstantBuffer(TEXT("FIsShadowConstants"), 5, EShaderStage::Pixel);
}

void FShadowRenderPass::PrepareCSMRenderState()
{
    StaticMeshIL = ShaderManager->GetInputLayoutByKey(L"StaticMeshVertexShader");
    CascadedShadowMapVS = ShaderManager->GetVertexShaderByKey(L"CascadedShadowMapVS");
    CascadedShadowMapPS = ShaderManager->GetPixelShaderByKey(L"CascadedShadowMapPS");

    Graphics->DeviceContext->IASetInputLayout(StaticMeshIL);
    Graphics->DeviceContext->VSSetShader(CascadedShadowMapVS, nullptr, 0);
    Graphics->DeviceContext->GSSetShader(CascadedShadowMapGS, nullptr, 0);

    // Note : PS만 언바인드할 뿐, UpdateLightBuffer에서 바인딩된 SRV 슬롯들은 그대로 남아 있음
    Graphics->DeviceContext->PSSetShader(nullptr, nullptr, 0);
    Graphics->DeviceContext->RSSetState(Graphics->RasterizerShadow);

    BufferManager->BindConstantBuffer(TEXT("FCascadeConstantBuffer"), 0, EShaderStage::Vertex);
    BufferManager->BindConstantBuffer(TEXT("FCascadeConstantBuffer"), 0, EShaderStage::Geometry);
    BufferManager->BindConstantBuffer(TEXT("FCascadeConstantBuffer"), 9, EShaderStage::Pixel);

}

void FShadowRenderPass::PrepareRenderArr()
{
    for (const auto Iter : TObjectRange<UStaticMeshComponent>())
    {
        if (!Cast<UGizmoBaseComponent>(Iter) && Iter->GetWorld() == GEngine->ActiveWorld)
        {
            if (Iter->GetOwner() && !Iter->GetOwner()->IsHidden())
            {
                StaticMeshComponents.Add(Iter);
            }
        }
    }
}

void FShadowRenderPass::UpdateIsShadowConstant(int32 IsShadow) const
{
    FIsShadowConstants ShadowData;
    ShadowData.bIsShadow = IsShadow;
    BufferManager->UpdateConstantBuffer(TEXT("FIsShadowConstants"), ShadowData);
}


void FShadowRenderPass::Render(ULightComponentBase* Light)
{
    
}

void FShadowRenderPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    const uint64 ShowFlag = Viewport->GetShowFlag();
    if (ShowFlag & EEngineShowFlags::SF_Shadow)
    {
        UpdateIsShadowConstant(1);
    }
    else
    {
        UpdateIsShadowConstant(0);
    }

    Graphics->DeviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    Graphics->DeviceContext->OMSetDepthStencilState(Graphics->DepthStencilState_Default, 1);
    
    for (const auto DirectionalLight : TObjectRange<UDirectionalLightComponent>())
    {
        // Cascade Shadow Map을 위한 ViewProjection Matrix 설정
            ShadowManager->UpdateCascadeMatrices(Viewport, DirectionalLight);

            PrepareCSMRenderState();
            FCascadeConstantBuffer CascadeData = {};
            uint32 NumCascades = ShadowManager->GetNumCasCades();
            for (uint32 Idx = 0; Idx < NumCascades; Idx++)
            {
                CascadeData.ViewProj[Idx] = ShadowManager->GetCascadeViewProjMatrix(Idx);
            }

            ShadowManager->BeginDirectionalShadowCascadePass(0);
            //RenderAllStaticMeshes(Viewport);

            RenderAllStaticMeshesForCSM(Viewport, CascadeData);

            Graphics->DeviceContext->GSSetShader(nullptr, nullptr, 0);
            Graphics->DeviceContext->RSSetViewports(0, nullptr);
            Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
       
    }
    PrepareRenderState();
    for (int Idx = 0 ; Idx < SpotLights.Num(); Idx++)
    {
        const auto& SpotLight = SpotLights[Idx];
        FShadowConstantBuffer ShadowData;
        FMatrix LightViewMatrix = SpotLight->GetViewMatrix();
        FMatrix LightProjectionMatrix = SpotLight->GetProjectionMatrix();
        ShadowData.ShadowViewProj = LightViewMatrix * LightProjectionMatrix;

        BufferManager->UpdateConstantBuffer(TEXT("FShadowConstantBuffer"), ShadowData);

        ShadowManager->BeginSpotShadowPass(Idx);
        RenderAllStaticMeshes(Viewport);
           
        Graphics->DeviceContext->RSSetViewports(0, nullptr);
        Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    }

    PrepareCubeMapRenderState();
    for (int Idx = 0 ; Idx < PointLights.Num(); Idx++)
    {
        
        ShadowManager->BeginPointShadowPass(Idx);
        RenderAllStaticMeshesForPointLight(Viewport, PointLights[Idx]);
           
        Graphics->DeviceContext->RSSetViewports(0, nullptr);
        Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    }
    Graphics->DeviceContext->GSSetShader(nullptr, nullptr, 0);
}


void FShadowRenderPass::ClearRenderArr()
{
    StaticMeshComponents.Empty();
}

void FShadowRenderPass::SetLightData(const TArray<class UPointLightComponent*>& InPointLights, const TArray<class USpotLightComponent*>& InSpotLights)
{
    PointLights = InPointLights;
    SpotLights = InSpotLights;
}

void FShadowRenderPass::RenderPrimitive(FStaticMeshRenderData* RenderData, const TArray<FStaticMaterial*> Materials, TArray<UMaterial*> OverrideMaterials, int32 SelectedSubMeshIndex)
{
    UINT Stride = sizeof(FStaticMeshVertex);
    UINT Offset = 0;

    FVertexInfo VertexInfo;
    BufferManager->CreateVertexBuffer(RenderData->ObjectName, RenderData->Vertices, VertexInfo);
    
    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &VertexInfo.VertexBuffer, &Stride, &Offset);

    FIndexInfo IndexInfo;
    BufferManager->CreateIndexBuffer(RenderData->ObjectName, RenderData->Indices, IndexInfo);
    if (IndexInfo.IndexBuffer)
    {
        Graphics->DeviceContext->IASetIndexBuffer(IndexInfo.IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
    }

    if (RenderData->MaterialSubsets.Num() == 0)
    {
        Graphics->DeviceContext->DrawIndexed(RenderData->Indices.Num(), 0, 0);
        return;
    }

    for (int SubMeshIndex = 0; SubMeshIndex < RenderData->MaterialSubsets.Num(); SubMeshIndex++)
    {
        uint32 MaterialIndex = RenderData->MaterialSubsets[SubMeshIndex].MaterialIndex;

        FSubMeshConstants SubMeshData = (SubMeshIndex == SelectedSubMeshIndex) ? FSubMeshConstants(true) : FSubMeshConstants(false);

        BufferManager->UpdateConstantBuffer(TEXT("FSubMeshConstants"), SubMeshData);

        if (!OverrideMaterials.IsEmpty() && OverrideMaterials.Num() >= static_cast<int32>(MaterialIndex) && OverrideMaterials[MaterialIndex] != nullptr)
        {
            MaterialUtils::UpdateMaterial(BufferManager, Graphics, OverrideMaterials[MaterialIndex]->GetMaterialInfo());
        }
        else if (!Materials.IsEmpty() && Materials.Num() >= static_cast<int32>(MaterialIndex) && Materials[MaterialIndex] != nullptr)
        {
            MaterialUtils::UpdateMaterial(BufferManager, Graphics, Materials[MaterialIndex]->Material->GetMaterialInfo());
        }
        else if (UMaterial* Mat = UAssetManager::Get().GetMaterial(RenderData->MaterialSubsets[SubMeshIndex].MaterialName))
        {
            MaterialUtils::UpdateMaterial(BufferManager, Graphics, Mat->GetMaterialInfo());
        }

        uint32 StartIndex = RenderData->MaterialSubsets[SubMeshIndex].IndexStart;
        uint32 IndexCount = RenderData->MaterialSubsets[SubMeshIndex].IndexCount;
        Graphics->DeviceContext->DrawIndexed(IndexCount, StartIndex, 0);
    }
}

void FShadowRenderPass::RenderAllStaticMeshes(const std::shared_ptr<FEditorViewportClient>& Viewport)
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

        FMatrix WorldMatrix = Comp->GetWorldMatrix();
        FVector4 UUIDColor = Comp->EncodeUUID() / 255.0f;
        const bool bIsSelected = (Engine && Engine->GetSelectedActor() == Comp->GetOwner());

        UpdateObjectConstant(WorldMatrix, UUIDColor, bIsSelected);

        RenderPrimitive(RenderData, Comp->GetStaticMesh()->GetMaterials(), Comp->GetOverrideMaterials(), Comp->GetselectedSubMeshIndex());
        
    }
}

void FShadowRenderPass::RenderAllStaticMeshesForCSM(const std::shared_ptr<FEditorViewportClient>& Viewport, FCascadeConstantBuffer FCasCadeData)
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
        FMatrix WorldMatrix = Comp->GetWorldMatrix();
        FCasCadeData.World = WorldMatrix;
        BufferManager->UpdateConstantBuffer(TEXT("FCascadeConstantBuffer"), FCasCadeData);

        RenderPrimitive(RenderData, Comp->GetStaticMesh()->GetMaterials(), Comp->GetOverrideMaterials(), Comp->GetselectedSubMeshIndex());

    }
}

void FShadowRenderPass::BindResourcesForSampling()
{
    ShadowManager->BindResourcesForSampling(static_cast<UINT>(EShaderSRVSlot::SRV_SpotLight),
        static_cast<UINT>(EShaderSRVSlot::SRV_DirectionalLight),
    10);
}

void FShadowRenderPass::RenderAllStaticMeshesForPointLight(const std::shared_ptr<FEditorViewportClient>& Viewport, UPointLightComponent*& PointLight)
{
    for (UStaticMeshComponent* Comp : StaticMeshComponents)
    {
        if (!Comp || !Comp->GetStaticMesh()) { continue; }

        FStaticMeshRenderData* RenderData = Comp->GetStaticMesh()->GetRenderData();
        if (RenderData == nullptr) { continue; }

        UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);

        FMatrix WorldMatrix = Comp->GetWorldMatrix();

        UpdateCubeMapConstantBuffer(PointLight, WorldMatrix);

        RenderPrimitive(RenderData, Comp->GetStaticMesh()->GetMaterials(), Comp->GetOverrideMaterials(), Comp->GetselectedSubMeshIndex());
    }
}

void FShadowRenderPass::PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
}

void FShadowRenderPass::CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
}

void FShadowRenderPass::CreateResource()
{
    HRESULT hr = ShaderManager->AddVertexShader(L"DepthOnlyVS", L"Shaders/DepthOnlyVS.hlsl", "mainVS");
    if (FAILED(hr))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create DepthOnlyVS shader!"));
    }
    hr = ShaderManager->AddVertexShader(L"DepthCubeMapVS", L"Shaders/DepthCubeMapVS.hlsl", "mainVS");
    if (FAILED(hr))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create DepthCubeMapVS shader!"));
    }

    hr = ShaderManager->AddGeometryShader(L"DepthCubeMapGS", L"Shaders/PointLightCubemapGS.hlsl", "mainGS");
    if (FAILED(hr))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create DepthCubeMapGS shader!"));
    }

    hr = ShaderManager->AddVertexShader(L"CascadedShadowMapVS", L"Shaders/CascadedShadowMap.hlsl", "mainVS");
    if (FAILED(hr))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create Cascaded ShadowMap Vertex shader!"));
    }

    hr = ShaderManager->AddGeometryShader(L"CascadedShadowMapGS", L"Shaders/CascadedShadowMap.hlsl", "mainGS");
    if (FAILED(hr))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create Cascaded ShadowMap Geometry shader!"));
    }

    hr = ShaderManager->AddPixelShader(L"CascadedShadowMapPS", L"Shaders/CascadedShadowMap.hlsl", "mainPS");
    if (FAILED(hr))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create Cascaded ShadowMap Pixel shader!"));
    }   


    StaticMeshIL = ShaderManager->GetInputLayoutByKey(L"StaticMeshVertexShader");
    DepthOnlyVS = ShaderManager->GetVertexShaderByKey(L"DepthOnlyVS");
    DepthOnlyPS = ShaderManager->GetPixelShaderByKey(L"DepthOnlyPS");

    CascadedShadowMapVS = ShaderManager->GetVertexShaderByKey(L"CascadedShadowMapVS");
    CascadedShadowMapGS = ShaderManager->GetGeometryShaderByKey(L"CascadedShadowMapGS");
    CascadedShadowMapPS = ShaderManager->GetPixelShaderByKey(L"CascadedShadowMapPS");
}


void FShadowRenderPass::PrepareCubeMapRenderState()
{
    /*auto*& DSV = Viewport->GetViewportResource()->GetDepthStencil(EResourceType::ERT_Scene)->DSV;*/
    // auto sm = PointLight->GetShadowMap();
    // auto*& DSV = sm[1].DSV;
    // Graphics->DeviceContext->ClearDepthStencilView(DSV,
    //     D3D11_CLEAR_DEPTH, 1.0f, 0);
    //Graphics->DeviceContext->ClearRenderTargetView(PointLight->DepthRTVArray, ClearColor);
    //Graphics->DeviceContext->OMSetRenderTargets(1, &PointLight->DepthRTVArray, DSV);

    DepthCubeMapVS = ShaderManager->GetVertexShaderByKey(L"DepthCubeMapVS");
    DepthCubeMapGS = ShaderManager->GetGeometryShaderByKey(L"DepthCubeMapGS");
    DepthOnlyPS = ShaderManager->GetPixelShaderByKey(L"DepthOnlyPS");

    Graphics->DeviceContext->VSSetShader(DepthCubeMapVS, nullptr, 0);
    Graphics->DeviceContext->IASetInputLayout(StaticMeshIL);
    
    Graphics->DeviceContext->GSSetShader(DepthCubeMapGS, nullptr, 0);
    
    Graphics->DeviceContext->PSSetShader(DepthOnlyPS, nullptr, 0);
    Graphics->DeviceContext->RSSetState(Graphics->RasterizerSolidBack);
    
    // VS, GS에 대한 상수버퍼 업데이트
    BufferManager->BindConstantBuffer(TEXT("FPointLightGSBuffer"), 0, EShaderStage::Geometry);
    BufferManager->BindConstantBuffer(TEXT("FShadowConstantBuffer"), 11, EShaderStage::Vertex);
    BufferManager->BindConstantBuffer(TEXT("FShadowConstantBuffer"), 11, EShaderStage::Pixel);

    //UpdateViewport(ShadowMapWidth, ShadowMapHeight);
    //Graphics->DeviceContext->RSSetViewports(1, &ShadowViewport);
}

void FShadowRenderPass::UpdateCubeMapConstantBuffer(UPointLightComponent*& PointLight,
    const FMatrix& WorldMatrix
    ) const
{
    FPointLightGSBuffer DepthCubeMapBuffer;
    DepthCubeMapBuffer.World = WorldMatrix;
    for (int32 Idx = 0; Idx < 6; ++Idx)
    {
        DepthCubeMapBuffer.ViewProj[Idx] = PointLight->GetViewMatrix(Idx) * PointLight->GetProjectionMatrix();
    }
    BufferManager->UpdateConstantBuffer(TEXT("FPointLightGSBuffer"), DepthCubeMapBuffer);
}

void FShadowRenderPass::RenderCubeMap(const std::shared_ptr<FEditorViewportClient>& Viewport, UPointLightComponent*& PointLight)
{
    //UpdateCubeMapConstantBuffer(PointLight);
    PrepareCubeMapRenderState();

}


