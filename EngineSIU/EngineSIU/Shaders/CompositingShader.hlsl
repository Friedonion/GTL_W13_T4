
Texture2D SceneTexture : register(t100);
Texture2D TranslucentTexture : register(t101);
Texture2D PP_PostProcessTexture : register(t102);
Texture2D EditorTexture : register(t103);
Texture2D EditorOverlayTexture : register(t104);
Texture2D DebugTexture : register(t106);
Texture2D CameraEffectTexture : register(t107);

SamplerState CompositingSampler : register(s1); // Linear Clamp

#define VMI_Lit_Gouraud      0
#define VMI_Lit_Lambert      1
#define VMI_Lit_BlinnPhong   2
#define VMI_Lit_SG           3
#define VMI_Unlit            4
#define VMI_Wireframe        5
#define VMI_SceneDepth       6
#define VMI_WorldNormal      7
#define VMI_WorldTangent     8
#define VMI_LightHeatMap     9

cbuffer ViewMode : register(b0)
{
    uint ViewMode; 
    float3 Padding;
}

cbuffer Gamma : register(b1)
{
    float GammaValue;
    float3 GammaPadding;
}

struct PS_Input
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD;
};

float4 main(PS_Input Input) : SV_TARGET
{
    float4 Scene = SceneTexture.Sample(CompositingSampler, Input.UV);
    Scene = pow(Scene, GammaValue);
    float4 Translucent = TranslucentTexture.Sample(CompositingSampler, Input.UV);
    float4 PostProcess = PP_PostProcessTexture.Sample(CompositingSampler, Input.UV);
    float4 Editor = EditorTexture.Sample(CompositingSampler, Input.UV);
    float4 EditorOverlay = EditorOverlayTexture.Sample(CompositingSampler, Input.UV);
    float4 Debug = DebugTexture.Sample(CompositingSampler, Input.UV);
    float4 CameraEffect = CameraEffectTexture.Sample(CompositingSampler, Input.UV);
    
    float4 FinalColor = Scene;
    if (ViewMode == VMI_LightHeatMap)
    {
        FinalColor = lerp(FinalColor, Debug, 0.5);
        FinalColor = lerp(FinalColor, Editor, Editor.a);
        FinalColor = lerp(FinalColor, Translucent, Translucent.a);
    }
    else
    {
        FinalColor = lerp(FinalColor, PostProcess, PostProcess.a);
        FinalColor = lerp(FinalColor, Editor, Editor.a);
        // TODO: 반투명 물체는 포스트 프로세싱을 어떻게 처리해야하는지 고민해야 함.
        FinalColor = lerp(FinalColor, Translucent, Translucent.a);
        FinalColor = lerp(FinalColor, EditorOverlay, EditorOverlay.a);
        FinalColor = lerp(FinalColor, CameraEffect, CameraEffect.a);
    }

    return FinalColor;
}
