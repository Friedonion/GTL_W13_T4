#include "PostProcessRenderPass.h"

#include "CameraEffectRenderPass.h"
#include "EngineBaseTypes.h"
#include "FogRenderPass.h"
#include "PostProcessCompositingPass.h"
#include "UnrealEd/EditorViewportClient.h"
#include "UObject/Object.h"
#include "PropertyEditor/ShowFlags.h"
#include "DepthOfFieldRenderPass.h"

FPostProcessRenderPass::FPostProcessRenderPass()
{
    FogRenderPass = AddRenderPass<FFogRenderPass>();
    CameraEffectRenderPass = AddRenderPass<FCameraEffectRenderPass>();
    PostProcessCompositingPass = AddRenderPass<FPostProcessCompositingPass>();
    DepthOfFieldRenderPass = AddRenderPass<FDepthOfFieldRenderPass>();
}

void FPostProcessRenderPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    PrepareRender(Viewport);

    const uint64 ShowFlag = Viewport->GetShowFlag();
    const EViewModeIndex ViewMode = Viewport->GetViewMode();

    if (ShowFlag & EEngineShowFlags::SF_Fog)
    {
        QUICK_SCOPE_CYCLE_COUNTER(FogPass_CPU)
        QUICK_GPU_SCOPE_CYCLE_COUNTER(FogPass_GPU, *GPUTimingManager)
        FogRenderPass->Render(Viewport);
    }

    if (ShowFlag & EEngineShowFlags::SF_DOF)
    {
        QUICK_SCOPE_CYCLE_COUNTER(DepthOfFieldPass_CPU)
        QUICK_GPU_SCOPE_CYCLE_COUNTER(DepthOfFieldPass_GPU, *GPUTimingManager)
        DepthOfFieldRenderPass->Render(Viewport);
    }

    // TODO: 포스트 프로세스 별로 각자의 렌더 타겟 뷰에 렌더하기

    /**
     * TODO: 반드시 씬에 먼저 반영되어야 하는 포스트 프로세싱 효과는 먼저 씬에 반영하고,
     *       그 외에는 렌더한 포스트 프로세싱 효과들을 이 시점에서 하나로 합친 후에, 다음에 올 컴포짓 과정에서 합성.
     */
    {
        CameraEffectRenderPass->Render(Viewport);
    }

    {
        QUICK_SCOPE_CYCLE_COUNTER(PostProcessCompositing_CPU)
        QUICK_GPU_SCOPE_CYCLE_COUNTER(PostProcessCompositing_GPU, *GPUTimingManager)
        PostProcessCompositingPass->Render(Viewport);
    }
    
    CleanUpRender(Viewport);
}

void FPostProcessRenderPass::PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
}

void FPostProcessRenderPass::CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
}
