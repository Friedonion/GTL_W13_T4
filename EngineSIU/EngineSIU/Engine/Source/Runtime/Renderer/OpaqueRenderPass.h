#pragma once
#include "RenderPassBase.h"
#include "EngineBaseTypes.h"
#include "Container/Set.h"

#include "Define.h"

struct FStaticMeshRenderData;
class FShadowManager;
class FDXDShaderManager;
class UWorld;
class UMaterial;
class FEditorViewportClient;
class UStaticMeshComponent;
struct FStaticMaterial;
class FShadowRenderPass;

class FOpaqueRenderPass : public FRenderPassBase
{
public:
    FOpaqueRenderPass() = default;
    virtual ~FOpaqueRenderPass() override = default;
    
    virtual void Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager) override;
    
    virtual void PrepareRenderArr() override;

    virtual void ClearRenderArr() override;

    virtual void Render(const std::shared_ptr<FEditorViewportClient>& Viewport) override;
    
    void UpdateLitUnlitConstant(int32 IsLit) const;
    
    void ChangeViewMode(EViewModeIndex ViewMode);
    
protected:
    virtual void PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport) override;
    virtual void CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport) override;

    virtual void CreateResource() override;

    void PrepareStaticMesh();
    void RenderStaticMesh(const std::shared_ptr<FEditorViewportClient>& Viewport);

    void PrepareSkeletalMesh();
    void RenderSkeletalMesh(const std::shared_ptr<FEditorViewportClient>& Viewport);
    
    void PrepareSkeletalMeshInstanced();
    void RenderSkeletalMeshInstanced(const std::shared_ptr<FEditorViewportClient>& Viewport);

    
    TArray<UStaticMeshComponent*> StaticMeshComponents;
    TArray<USkeletalMeshComponent*> SkeletalMeshComponents;

    ID3D11VertexShader* VertexShader_StaticMesh = nullptr;
    ID3D11InputLayout* InputLayout_StaticMesh = nullptr;

    ID3D11VertexShader* VertexShader_SkeletalMesh = nullptr;
    ID3D11VertexShader* VertexShader_SkeletalMeshInstanced = nullptr;
    ID3D11InputLayout* InputLayout_SkeletalMesh = nullptr;

    ID3D11PixelShader* PixelShader = nullptr;
};
