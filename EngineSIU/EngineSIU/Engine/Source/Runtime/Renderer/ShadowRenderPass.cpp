#include "ShadowRenderPass.h"

#include "ShadowManager.h"
#include "BaseGizmos/GizmoBaseComponent.h"
#include "Components/Light/LightComponent.h"
#include "Components/Light/PointLightComponent.h"
#include "D3D11RHI/DXDBufferManager.h"
#include "D3D11RHI/GraphicDevice.h"
#include "D3D11RHI/DXDShaderManager.h"
#include "Components/Light/DirectionalLightComponent.h"
#include "Components/Light/SpotLightComponent.h"
#include "Engine/EditorEngine.h"
#include "Engine/Engine.h"
#include "UnrealEd/EditorViewportClient.h"
#include "UObject/Casts.h"
#include "UObject/UObjectIterator.h"
#include "Editor/PropertyEditor/ShowFlags.h"
#include "Engine/AssetManager.h"
#include "Engine/SkeletalMesh.h"

class UEditorEngine;
class UStaticMeshComponent;

void FShadowRenderPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    FRenderPassBase::Initialize(InBufferManager, InGraphics, InShaderManager);
}

void FShadowRenderPass::InitializeShadowManager(class FShadowManager* InShadowManager)
{
    ShadowManager = InShadowManager;
}

void FShadowRenderPass::PrepareSpotLightRenderState()
{
    Graphics->DeviceContext->PSSetShader(nullptr, nullptr, 0);
    Graphics->DeviceContext->RSSetState(Graphics->RasterizerShadow);
    
    BufferManager->BindConstantBuffer(TEXT("FShadowConstantBuffer"), 11, EShaderStage::Vertex);
    BufferManager->BindConstantBuffer(TEXT("FIsShadowConstants"), 5, EShaderStage::Pixel);
}

void FShadowRenderPass::PrepareCSMRenderState()
{
    CascadedShadowMapGS = ShaderManager->GetGeometryShaderByKey(L"CascadedShadowMapGS");
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
        if (Iter->GetWorld() != GEngine->ActiveWorld)
        {
            continue;
        }
        if (!Iter->GetOwner() || Iter->GetOwner()->IsHidden())
        {
            continue;
        }

        if (UStaticMeshComponent* StaticMeshComp = Cast<UStaticMeshComponent>(Iter))
        {
            if (Iter->IsA<UGizmoBaseComponent>())
            {
                continue;
            }

            StaticMeshComponents.Add(StaticMeshComp);       
        }
    }

    for (const auto Iter : TObjectRange<USkeletalMeshComponent>())
    {
        if (Iter->GetWorld() != GEngine->ActiveWorld)
        {
            continue;
        }
        if (!Iter->GetOwner() || Iter->GetOwner()->IsHidden())
        {
            continue;
        }

        if (USkeletalMeshComponent* SkeletalMeshComp = Cast<USkeletalMeshComponent>(Iter))
        {
            SkeletalMeshComponents.Add(SkeletalMeshComp);
        }
    }
}

void FShadowRenderPass::UpdateIsShadowConstant(int32 IsShadow) const
{
    FIsShadowConstants ShadowData;
    ShadowData.bIsShadow = IsShadow;
    BufferManager->UpdateConstantBuffer(TEXT("FIsShadowConstants"), ShadowData);
}

void FShadowRenderPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    PrepareRender(Viewport);

    // Directional Light
    PrepareCSMRenderState();
    for (const auto DirectionalLight : TObjectRange<UDirectionalLightComponent>())
    {
        // Cascade Shadow Map을 위한 ViewProjection Matrix 설정
        ShadowManager->UpdateCascadeMatrices(Viewport, DirectionalLight);

        FCascadeConstantBuffer CascadeData = {};
        uint32 NumCascades = ShadowManager->GetNumCasCades();
        for (uint32 Idx = 0; Idx < NumCascades; Idx++)
        {
            CascadeData.ViewProj[Idx] = ShadowManager->GetCascadeViewProjMatrix(Idx);
        }

        ShadowManager->BeginDirectionalShadowCascadePass(0);

        RenderAllMeshesForCSM(Viewport, CascadeData);
    }
    Graphics->DeviceContext->GSSetShader(nullptr, nullptr, 0);
    Graphics->DeviceContext->RSSetViewports(0, nullptr);
    Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);

    // Spot Light
    PrepareSpotLightRenderState();
    for (int Idx = 0 ; Idx < SpotLights.Num(); Idx++)
    {
        const auto& SpotLight = SpotLights[Idx];
        FShadowConstantBuffer ShadowData;
        FMatrix LightViewMatrix = SpotLight->GetViewMatrix();
        FMatrix LightProjectionMatrix = SpotLight->GetProjectionMatrix();
        ShadowData.ShadowViewProj = LightViewMatrix * LightProjectionMatrix;

        BufferManager->UpdateConstantBuffer(TEXT("FShadowConstantBuffer"), ShadowData);

        ShadowManager->BeginSpotShadowPass(Idx);
        RenderAllMeshesForSpotLight(Viewport);
    }
    Graphics->DeviceContext->RSSetViewports(0, nullptr);
    Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);

    // Point Light
    PreparePointLightRenderState();
    for (int Idx = 0 ; Idx < PointLights.Num(); Idx++)
    {
        ShadowManager->BeginPointShadowPass(Idx);
        RenderAllMeshesForPointLight(Viewport, PointLights[Idx]);
    }
    Graphics->DeviceContext->RSSetViewports(0, nullptr);
    Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);

    CleanUpRender(Viewport);
}

void FShadowRenderPass::ClearRenderArr()
{
    StaticMeshComponents.Empty();
    SkeletalMeshComponents.Empty();
}

void FShadowRenderPass::SetLightData(const TArray<class UPointLightComponent*>& InPointLights, const TArray<class USpotLightComponent*>& InSpotLights)
{
    PointLights = InPointLights;
    SpotLights = InSpotLights;
}

void FShadowRenderPass::RenderAllMeshesForSpotLight(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    StaticMeshIL = ShaderManager->GetInputLayoutByKey(L"StaticMeshVertexShader");
    DepthOnlyVS = ShaderManager->GetVertexShaderByKey(L"DepthOnlyVS_SM");
    
    Graphics->DeviceContext->IASetInputLayout(StaticMeshIL);
    Graphics->DeviceContext->VSSetShader(DepthOnlyVS, nullptr, 0);

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

        RenderStaticMesh_Internal(RenderData, Comp->GetStaticMesh()->GetMaterials(), Comp->GetOverrideMaterials(), Comp->GetselectedSubMeshIndex());
    }

    ID3D11InputLayout* TempIL = ShaderManager->GetInputLayoutByKey(L"SkeletalMeshVertexShader");
    ID3D11VertexShader* TempVS = ShaderManager->GetVertexShaderByKey(L"DepthOnlyVS_SKM");
    
    Graphics->DeviceContext->IASetInputLayout(TempIL);
    Graphics->DeviceContext->VSSetShader(TempVS, nullptr, 0);
    
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

void FShadowRenderPass::RenderAllMeshesForCSM(const std::shared_ptr<FEditorViewportClient>& Viewport, FCascadeConstantBuffer FCasCadeData)
{
    StaticMeshIL = ShaderManager->GetInputLayoutByKey(L"StaticMeshVertexShader");
    CascadedShadowMapVS = ShaderManager->GetVertexShaderByKey(L"CascadedShadowMapVS_SM");
    
    Graphics->DeviceContext->IASetInputLayout(StaticMeshIL);
    Graphics->DeviceContext->VSSetShader(CascadedShadowMapVS, nullptr, 0);
    
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
        FCasCadeData.World = WorldMatrix;
        BufferManager->UpdateConstantBuffer(TEXT("FCascadeConstantBuffer"), FCasCadeData);

        RenderStaticMesh_Internal(RenderData, Comp->GetStaticMesh()->GetMaterials(), Comp->GetOverrideMaterials(), Comp->GetselectedSubMeshIndex());
    }

    ID3D11InputLayout* TempIL = ShaderManager->GetInputLayoutByKey(L"SkeletalMeshVertexShader");
    ID3D11VertexShader* TempVS = ShaderManager->GetVertexShaderByKey(L"CascadedShadowMapVS_SKM");

    Graphics->DeviceContext->IASetInputLayout(TempIL);
    Graphics->DeviceContext->VSSetShader(TempVS, nullptr, 0);
    
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
        FCasCadeData.World = WorldMatrix;
        BufferManager->UpdateConstantBuffer(TEXT("FCascadeConstantBuffer"), FCasCadeData);

        UpdateBones(Comp);

        RenderSkeletalMesh_Internal(RenderData);
    }
}

void FShadowRenderPass::BindResourcesForSampling()
{
    ShadowManager->BindResourcesForSampling(
        static_cast<UINT>(EShaderSRVSlot::SRV_SpotLight),
        static_cast<UINT>(EShaderSRVSlot::SRV_DirectionalLight),
        10
    );
}

void FShadowRenderPass::RenderAllMeshesForPointLight(const std::shared_ptr<FEditorViewportClient>& Viewport, UPointLightComponent*& PointLight)
{
    StaticMeshIL = ShaderManager->GetInputLayoutByKey(L"StaticMeshVertexShader");
    DepthCubeMapVS = ShaderManager->GetVertexShaderByKey(L"DepthCubeMapVS_SM");

    Graphics->DeviceContext->IASetInputLayout(StaticMeshIL);
    Graphics->DeviceContext->VSSetShader(DepthCubeMapVS, nullptr, 0);
    
    for (UStaticMeshComponent* Comp : StaticMeshComponents)
    {
        if (!Comp || !Comp->GetStaticMesh()) { continue; }

        FStaticMeshRenderData* RenderData = Comp->GetStaticMesh()->GetRenderData();
        if (RenderData == nullptr) { continue; }

        FMatrix WorldMatrix = Comp->GetWorldMatrix();
        UpdateCubeMapConstantBuffer(PointLight, WorldMatrix);

        RenderStaticMesh_Internal(RenderData, Comp->GetStaticMesh()->GetMaterials(), Comp->GetOverrideMaterials(), Comp->GetselectedSubMeshIndex());
    }

    ID3D11InputLayout* TempIL = ShaderManager->GetInputLayoutByKey(L"SkeletalMeshVertexShader");
    ID3D11VertexShader* TempVS = ShaderManager->GetVertexShaderByKey(L"DepthCubeMapVS_SKM");

    Graphics->DeviceContext->IASetInputLayout(TempIL);
    Graphics->DeviceContext->VSSetShader(TempVS, nullptr, 0);
    
    for (USkeletalMeshComponent* Comp : SkeletalMeshComponents)
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
        UpdateCubeMapConstantBuffer(PointLight, WorldMatrix);

        UpdateBones(Comp);

        RenderSkeletalMesh_Internal(RenderData);
    }
}

void FShadowRenderPass::PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
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
    
    BufferManager->BindStructuredBufferSRV(TEXT("BoneBuffer"), 1, EShaderStage::Vertex);
    BufferManager->BindConstantBuffer(TEXT("FCPUSkinningConstants"), 2, EShaderStage::Vertex);

    FCPUSkinningConstants CPUSkinningData;
    CPUSkinningData.bCPUSkinning = USkeletalMeshComponent::GetCPUSkinning();
    BufferManager->UpdateConstantBuffer(TEXT("FCPUSkinningConstants"), CPUSkinningData);
}

void FShadowRenderPass::CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    Graphics->DeviceContext->RSSetViewports(0, nullptr);
    Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    Graphics->DeviceContext->GSSetShader(nullptr, nullptr, 0);
}

void FShadowRenderPass::CreateResource()
{
    HRESULT hr = ShaderManager->AddVertexShader(L"DepthOnlyVS_SM", L"Shaders/DepthOnlyVS.hlsl", "mainVS_SM");
    if (FAILED(hr))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create DepthOnlyVS shader!"));
    }
    
    hr = ShaderManager->AddVertexShader(L"DepthOnlyVS_SKM", L"Shaders/DepthOnlyVS.hlsl", "mainVS_SKM");
    if (FAILED(hr))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create DepthOnlyVS shader!"));
    }
    
    hr = ShaderManager->AddVertexShader(L"DepthCubeMapVS_SM", L"Shaders/DepthCubeMapVS.hlsl", "mainVS_SM");
    if (FAILED(hr))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create DepthCubeMapVS shader!"));
    }

    hr = ShaderManager->AddVertexShader(L"DepthCubeMapVS_SKM", L"Shaders/DepthCubeMapVS.hlsl", "mainVS_SKM");
    if (FAILED(hr))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create DepthCubeMapVS shader!"));
    }

    hr = ShaderManager->AddGeometryShader(L"DepthCubeMapGS", L"Shaders/PointLightCubemapGS.hlsl", "mainGS");
    if (FAILED(hr))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create DepthCubeMapGS shader!"));
    }

    hr = ShaderManager->AddVertexShader(L"CascadedShadowMapVS_SM", L"Shaders/CascadedShadowMap.hlsl", "mainVS_SM");
    if (FAILED(hr))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create Cascaded ShadowMap Vertex shader!"));
    }

    hr = ShaderManager->AddVertexShader(L"CascadedShadowMapVS_SKM", L"Shaders/CascadedShadowMap.hlsl", "mainVS_SKM");
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
}

void FShadowRenderPass::PreparePointLightRenderState()
{
    DepthCubeMapGS = ShaderManager->GetGeometryShaderByKey(L"DepthCubeMapGS");
    Graphics->DeviceContext->GSSetShader(DepthCubeMapGS, nullptr, 0);

    Graphics->DeviceContext->PSSetShader(nullptr, nullptr, 0);
    Graphics->DeviceContext->RSSetState(Graphics->RasterizerSolidBack);
    
    // VS, GS에 대한 상수버퍼 업데이트
    BufferManager->BindConstantBuffer(TEXT("FPointLightGSBuffer"), 0, EShaderStage::Geometry);
    BufferManager->BindConstantBuffer(TEXT("FShadowConstantBuffer"), 11, EShaderStage::Vertex);
}

void FShadowRenderPass::UpdateCubeMapConstantBuffer(UPointLightComponent*& PointLight, const FMatrix& WorldMatrix) const
{
    FPointLightGSBuffer DepthCubeMapBuffer;
    DepthCubeMapBuffer.World = WorldMatrix;
    for (int32 Idx = 0; Idx < 6; ++Idx)
    {
        DepthCubeMapBuffer.ViewProj[Idx] = PointLight->GetViewMatrix(Idx) * PointLight->GetProjectionMatrix();
    }
    BufferManager->UpdateConstantBuffer(TEXT("FPointLightGSBuffer"), DepthCubeMapBuffer);
}
