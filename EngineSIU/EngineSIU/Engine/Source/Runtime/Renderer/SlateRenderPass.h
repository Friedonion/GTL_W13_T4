
#pragma once
#include "RenderPassBase.h"
#include "EngineBaseTypes.h"

#include "Define.h"

struct FSlateTransform
{
    FVector2D Scale;
    FVector2D Offset;
};

class FSlateRenderPass : public FRenderPassBase
{
public:
    FSlateRenderPass() = default;
    virtual ~FSlateRenderPass() override = default;

    virtual void Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager) override;
    
    virtual void PrepareRenderArr() override;

    virtual void Render(const std::shared_ptr<FEditorViewportClient>& Viewport) override;

    virtual void ClearRenderArr() override;

    void CreateBuffer();

protected:
    virtual void PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport) override;
    virtual void CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport) override;

    virtual void CreateResource() override;
};
