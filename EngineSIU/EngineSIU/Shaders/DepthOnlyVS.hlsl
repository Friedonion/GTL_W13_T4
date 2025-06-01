// Depth Only Vertex Shader

#include "ShaderRegisters.hlsl"

StructuredBuffer<float4x4> BoneMatrices : register(t1);

cbuffer FCPUSkinningConstants : register(b2)
{
    bool bCPUSkinning;
    float3 pad0;
}

cbuffer FShadowConstantBuffer : register(b11)
{
    row_major matrix ShadowViewProj;
};

float4 mainVS_SM(VS_INPUT_StaticMesh Input) : SV_POSITION
{
    float4 pos = mul(float4(Input.Position, 1.0f), WorldMatrix);
    pos = mul(pos, ShadowViewProj);
    return pos;
}

float4 mainVS_SKM(VS_INPUT_SkeletalMesh Input) : SV_POSITION
{
    float4 SkinnedPosition = float4(0, 0, 0, 0);
    
    if (bCPUSkinning)
    {
        SkinnedPosition = float4(Input.Position, 1.0f);
    }
    else
    {    
        // 가중치 합산
        float TotalWeight = 0.0f;
    
        for (int i = 0; i < 4; ++i)
        {
            float Weight = Input.BoneWeights[i];
            TotalWeight += Weight;
        
            if (Weight > 0.0f)
            {
                uint BoneIdx = Input.BoneIndices[i];
                float4 Pos = mul(float4(Input.Position, 1.0f), BoneMatrices[BoneIdx]);
            
                SkinnedPosition += Weight * Pos;
            }
        }
    
        // 가중치 예외 처리
        if (TotalWeight < 0.001f)
        {
            SkinnedPosition = float4(Input.Position, 1.0f);
        }
        else if (abs(TotalWeight - 1.0f) > 0.001f && TotalWeight > 0.001f)
        {
            // 가중치 합이 1이 아닌 경우 정규화
            SkinnedPosition /= TotalWeight;
        }
    }

    float4 pos = mul(SkinnedPosition, WorldMatrix);
    pos = mul(pos, ShadowViewProj);
    
    return pos;
}
