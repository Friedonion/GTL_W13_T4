
#include "ShaderRegisters.hlsl"

Texture2D SceneDepthTexture : register(t99);

SamplerState Sampler : register(s3); // Point Clamp

cbuffer MaterialConstants : register(b1)
{
    FMaterial Material;
}

cbuffer ViewportSizeBuffer : register(b2)
{
    float2 ViewportSize;
    float2 Padding;
}

float3 ReconstructWorldPosition(float2 UV, float Z)
{
    float2 NDC;
    NDC.x = UV.x * 2.0f - 1.0f;
    NDC.y = (1.0f - UV.y) * 2.0f - 1.0f;

    float4 ClipPosition = float4(NDC, Z, 1.0f);
    
    float4 ViewPosition = mul(ClipPosition, InvProjectionMatrix);
    ViewPosition /= ViewPosition.w;
    
    float4 WorldPosition = mul(ViewPosition, InvViewMatrix);
    WorldPosition /= WorldPosition.w;

    return WorldPosition.xyz;
}

bool IsShaded(float3 SceneWorldPosition, float3 GizmoWorldPosition, float3 WorldViewPosition)
{
    const float Bias = 0.01f;

    float SceneDistance = length(SceneWorldPosition - WorldViewPosition);
    float GizmoDistance = length(GizmoWorldPosition - WorldViewPosition);

    return GizmoDistance > SceneDistance - Bias;
}

float4 mainPS(PS_INPUT_CommonMesh Input) : SV_Target
{
    float4 FinalColor = float4(Material.DiffuseColor, 1);
    
    if (bIsSelected)
    {
        FinalColor += float4(0.5f, 0.5f, 0.5f, 1); // 선택된 경우 강조
    }
    else
    {
        float2 DepthUV = Input.Position.xy / ViewportSize.xy;
        float Z = SceneDepthTexture.Sample(Sampler, DepthUV).r;
        float3 SceneWorldPosition = ReconstructWorldPosition(DepthUV, Z);

        if (IsShaded(SceneWorldPosition, Input.WorldPosition, ViewWorldLocation))
        {
            float GridSize = 3.f;
            float2 GridCoord = floor(Input.Position.xy / GridSize);
            bool bIsBlack = fmod(GridCoord.x + GridCoord.y, 2.0) < 1.0;
            if (bIsBlack)
            {
                return float4(0, 0, 0, 1);
            }
            
            FinalColor.xyz *= 0.15f; 
        }
        else
        {
            float3 WorldNormal = normalize(Input.WorldNormal);
            float3 LightDirection = normalize(float3(1, 2, 4));
            float Cos = max(dot(WorldNormal, LightDirection), 0.5f); // Fake Ambient
            
            FinalColor.xyz *= Cos;
        }
    }
       
    return FinalColor;
}
