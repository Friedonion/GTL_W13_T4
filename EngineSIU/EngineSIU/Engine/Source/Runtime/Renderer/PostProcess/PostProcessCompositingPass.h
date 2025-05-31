#pragma once
#include "RenderPassBase.h"
#include "LevelEditor/SlateAppMessageHandler.h"

class FPostProcessCompositingPass : public FRenderPassBase
{
public:
    FPostProcessCompositingPass() = default;
    virtual ~FPostProcessCompositingPass() override = default;
    
    virtual void Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManage) override;
    
    virtual void PrepareRenderArr() override;

    virtual void Render(const std::shared_ptr<FEditorViewportClient>& Viewport) override;

    virtual void ClearRenderArr() override;
    
protected:
    virtual void PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport) override;
    virtual void CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport) override;
};
