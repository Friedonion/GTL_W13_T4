#include "RenderPassBase.h"

#include "Define.h"
#include "RendererHelpers.h"
#include "Components/SkeletalMeshComponent.h"
#include "D3D11RHI/DXDBufferManager.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Asset/StaticMeshAsset.h"

FRenderPassBase::FRenderPassBase()
    : BufferManager(nullptr)
    , Graphics(nullptr)
    , ShaderManager(nullptr)
    , GPUTimingManager(nullptr)
{
    GPUTimingManager = FEngineLoop::Renderer.GPUTimingManager;
}

FRenderPassBase::~FRenderPassBase()
{
    Release();
}

void FRenderPassBase::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManage)
{
    BufferManager = InBufferManager;
    Graphics = InGraphics;
    ShaderManager = InShaderManage;

    CreateResource();

    for (IRenderPass* RenderPass : ChildRenderPasses)
    {
        RenderPass->Initialize(BufferManager, Graphics, ShaderManager);
    }
}

void FRenderPassBase::PrepareRenderArr()
{
    for (IRenderPass* RenderPass : ChildRenderPasses)
    {
        RenderPass->PrepareRenderArr();
    }
}

void FRenderPassBase::ClearRenderArr()
{
    for (IRenderPass* RenderPass : ChildRenderPasses)
    {
        RenderPass->ClearRenderArr();
    }
}

void FRenderPassBase::UpdateObjectConstant(const FMatrix& WorldMatrix, const FVector4& UUIDColor, bool bIsSelected) const
{
    FObjectConstantBuffer ObjectData = {};
    ObjectData.WorldMatrix = WorldMatrix;
    ObjectData.InverseTransposedWorld = FMatrix::Transpose(FMatrix::Inverse(WorldMatrix));
    ObjectData.UUIDColor = UUIDColor;
    ObjectData.bIsSelected = bIsSelected;
    
    BufferManager->UpdateConstantBuffer(TEXT("FObjectConstantBuffer"), ObjectData);
}

void FRenderPassBase::UpdateObjectConstantInstanced(int32 NumBones, const TArray<FMatrix>& WorldMatrix, int32 InstanceCount) const
{
    struct alignas(16) FObjectConstantBufferRaw
    {
        int32 NumBones;
        int32 Pad0[3];
        FObjectConstantBufferInstanced InstanceMatrices[FRenderer::MaxNumInstances];
    } ConstantBuffer = {};

    ConstantBuffer.NumBones = NumBones;

    for (int32 i = 0; i < InstanceCount; ++i)
    {
        FObjectConstantBufferInstanced ObjectData = {};
        ObjectData.WorldMatrix = WorldMatrix[i];
        ObjectData.InverseTransposedWorld = FMatrix::Transpose(FMatrix::Inverse(WorldMatrix[i]));
        ConstantBuffer.InstanceMatrices[i] = ObjectData;
    }

    BufferManager->UpdateConstantBuffer(TEXT("FObjectConstantBufferInstanced"), ConstantBuffer);
}

void FRenderPassBase::RenderStaticMesh_Internal(const FStaticMeshRenderData* RenderData, TArray<FStaticMaterial*> Materials, TArray<UMaterial*> OverrideMaterials, int32 SelectedSubMeshIndex)
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

    for (int32 SubMeshIndex = 0; SubMeshIndex < RenderData->MaterialSubsets.Num(); SubMeshIndex++)
    {
        int32 MaterialIndex = RenderData->MaterialSubsets[SubMeshIndex].MaterialIndex;

        FSubMeshConstants SubMeshData = (SubMeshIndex == SelectedSubMeshIndex) ? FSubMeshConstants(true) : FSubMeshConstants(false);

        BufferManager->UpdateConstantBuffer(TEXT("FSubMeshConstants"), SubMeshData);

        if (!OverrideMaterials.IsEmpty() && OverrideMaterials.Num() >= MaterialIndex && OverrideMaterials[MaterialIndex] != nullptr)
        {
            MaterialUtils::UpdateMaterial(BufferManager, Graphics, OverrideMaterials[MaterialIndex]->GetMaterialInfo());
        }
        else if (!Materials.IsEmpty() && Materials.Num() >= MaterialIndex && Materials[MaterialIndex] != nullptr)
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

void FRenderPassBase::RenderStaticMeshInstanced_Internal(const FStaticMeshRenderData* RenderData, int32 InstanceCount, TArray<FStaticMaterial*> Materials, TArray<UMaterial*> OverrideMaterials, int32 SelectedSubMeshIndex)
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
        Graphics->DeviceContext->DrawIndexedInstanced(IndexCount, InstanceCount, StartIndex, 0, 0);
    }
}

void FRenderPassBase::RenderSkeletalMesh_Internal(const FSkeletalMeshRenderData* RenderData)
{
    constexpr UINT Stride = sizeof(FSkeletalMeshVertex);
    constexpr UINT Offset = 0;

    FCPUSkinningConstants CPUSkinningData;
    CPUSkinningData.bCPUSkinning = USkeletalMeshComponent::GetCPUSkinning();
    BufferManager->UpdateConstantBuffer(TEXT("FCPUSkinningConstants"), CPUSkinningData);
    
    FVertexInfo VertexInfo;
    if (CPUSkinningData.bCPUSkinning)
    {
        BufferManager->CreateDynamicVertexBuffer(RenderData->ObjectName, RenderData->Vertices, VertexInfo);
        BufferManager->UpdateDynamicVertexBuffer(RenderData->ObjectName, RenderData->Vertices);
    }
    else
    {
        BufferManager->CreateVertexBuffer(RenderData->ObjectName, RenderData->Vertices, VertexInfo);
    }
    
    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &VertexInfo.VertexBuffer, &Stride, &Offset);
    
    FIndexInfo IndexInfo;
    BufferManager->CreateIndexBuffer(RenderData->ObjectName, RenderData->Indices, IndexInfo);
    if (IndexInfo.IndexBuffer)
    {
        Graphics->DeviceContext->IASetIndexBuffer(IndexInfo.IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
    }
    else
    {
        Graphics->DeviceContext->Draw(RenderData->Vertices.Num(), 0);
        return;
    }

    for (int SubMeshIndex = 0; SubMeshIndex < RenderData->MaterialSubsets.Num(); SubMeshIndex++)
    {
        FName MaterialName = RenderData->MaterialSubsets[SubMeshIndex].MaterialName;
        UMaterial* Material = UAssetManager::Get().GetMaterial(MaterialName);
        FMaterialInfo MaterialInfo = Material->GetMaterialInfo();
        MaterialUtils::UpdateMaterial(BufferManager, Graphics, MaterialInfo);

        const uint32 StartIndex = RenderData->MaterialSubsets[SubMeshIndex].IndexStart;
        const uint32 IndexCount = RenderData->MaterialSubsets[SubMeshIndex].IndexCount; 
        Graphics->DeviceContext->DrawIndexed(IndexCount, StartIndex, 0);
    }
}

void FRenderPassBase::RenderSkeletalMeshInstanced_Internal(const FSkeletalMeshRenderData* RenderData, int32 InstanceCount, int32 InstanceStartLocation)
{
    constexpr UINT Stride = sizeof(FSkeletalMeshVertex);
    constexpr UINT Offset = 0;

    FVertexInfo VertexInfo;
    BufferManager->CreateVertexBuffer(RenderData->ObjectName, RenderData->Vertices, VertexInfo);

    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &VertexInfo.VertexBuffer, &Stride, &Offset);

    FIndexInfo IndexInfo;
    BufferManager->CreateIndexBuffer(RenderData->ObjectName, RenderData->Indices, IndexInfo);
    if (IndexInfo.IndexBuffer)
    {
        Graphics->DeviceContext->IASetIndexBuffer(IndexInfo.IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
    }
    else
    {
        Graphics->DeviceContext->DrawInstanced(RenderData->Vertices.Num(), InstanceCount, 0, InstanceStartLocation);
        return;
    }

    for (int SubMeshIndex = 0; SubMeshIndex < RenderData->MaterialSubsets.Num(); SubMeshIndex++)
    {
        FName MaterialName = RenderData->MaterialSubsets[SubMeshIndex].MaterialName;
        UMaterial* Material = UAssetManager::Get().GetMaterial(MaterialName);
        FMaterialInfo MaterialInfo = Material->GetMaterialInfo();
        MaterialUtils::UpdateMaterial(BufferManager, Graphics, MaterialInfo);

        const uint32 StartIndex = RenderData->MaterialSubsets[SubMeshIndex].IndexStart;
        const uint32 IndexCount = RenderData->MaterialSubsets[SubMeshIndex].IndexCount;
        Graphics->DeviceContext->DrawIndexedInstanced(IndexCount, InstanceCount, StartIndex, 0, InstanceStartLocation);
    }
}

void FRenderPassBase::UpdateBones(const USkeletalMeshComponent* SkeletalMeshComponent)
{
    if (!SkeletalMeshComponent ||
        !SkeletalMeshComponent->GetSkeletalMeshAsset() ||
        !SkeletalMeshComponent->GetSkeletalMeshAsset()->GetSkeleton() ||
        USkeletalMeshComponent::GetCPUSkinning())
    {
        return;
    }

    // Skeleton 정보 가져오기
    const USkeletalMesh* SkeletalMesh = SkeletalMeshComponent->GetSkeletalMeshAsset();
    const FReferenceSkeleton& RefSkeleton = SkeletalMesh->GetSkeleton()->GetReferenceSkeleton();
    const int32 BoneNum = RefSkeleton.RawRefBoneInfo.Num();

    // 현재 애니메이션 본 행렬 계산
    TArray<FMatrix> CurrentGlobalBoneMatrices;
    SkeletalMeshComponent->GetCurrentGlobalBoneMatrices(CurrentGlobalBoneMatrices);
    
    // 최종 스키닝 행렬 계산
    TArray<FMatrix> FinalBoneMatrices;
    FinalBoneMatrices.SetNum(BoneNum);
    
    for (int32 BoneIndex = 0; BoneIndex < BoneNum; ++BoneIndex)
    {
        FinalBoneMatrices[BoneIndex] = RefSkeleton.InverseBindPoseMatrices[BoneIndex] * CurrentGlobalBoneMatrices[BoneIndex];
        FinalBoneMatrices[BoneIndex] = FMatrix::Transpose(FinalBoneMatrices[BoneIndex]);
    }

    BufferManager->UpdateStructuredBuffer(TEXT("BoneBuffer"), FinalBoneMatrices);
}

// 무조건 동일한 USKeletalMesh를 사용하는 컴포넌트가 와야합니다.
void FRenderPassBase::UpdateBonesInstanced(const TArray<USkeletalMeshComponent*>& SkeletalMeshComponents)
{
    if (SkeletalMeshComponents.IsEmpty() || USkeletalMeshComponent::GetCPUSkinning())
    {
        return;
    }
    TArray<FMatrix> FinalBoneMatrices;
    FinalBoneMatrices.Reserve(SkeletalMeshComponents.Num());
    for (const USkeletalMeshComponent* SkeletalMeshComponent : SkeletalMeshComponents)
    {
        if (!SkeletalMeshComponent || !SkeletalMeshComponent->GetSkeletalMeshAsset() || !SkeletalMeshComponent->GetSkeletalMeshAsset()->GetSkeleton())
        {
            continue;
        }
        const FReferenceSkeleton& RefSkeleton = SkeletalMeshComponent->GetSkeletalMeshAsset()->GetSkeleton()->GetReferenceSkeleton();
        const int32 BoneNum = RefSkeleton.RawRefBoneInfo.Num();

        TArray<FMatrix> CurrentGlobalBoneMatrices;
        SkeletalMeshComponent->GetCurrentGlobalBoneMatrices(CurrentGlobalBoneMatrices);

        for (int32 BoneIndex = 0; BoneIndex < BoneNum; ++BoneIndex)
        {
            FMatrix FinalMatrix = RefSkeleton.InverseBindPoseMatrices[BoneIndex] * CurrentGlobalBoneMatrices[BoneIndex];
            FinalMatrix = FMatrix::Transpose(FinalMatrix);
            FinalBoneMatrices.Add(FinalMatrix);
        }
    }
    BufferManager->UpdateStructuredBuffer(TEXT("BoneBufferInstanced"), FinalBoneMatrices);
}

void FRenderPassBase::Release()
{
    for (const IRenderPass* RenderPass : ChildRenderPasses)
    {
        delete RenderPass;
    }
}
