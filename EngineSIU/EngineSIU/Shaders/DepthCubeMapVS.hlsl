// Depth Only Vertex Shader
#define NUM_FACES 6 // 1개 삼각형 당 6개의 Depth용 버텍스 필요

struct VS_INPUT_StaticMesh
{
    float3 Position : POSITION;
    float4 Color : COLOR;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
    float2 UV : TEXCOORD;
};

struct VS_INPUT_SkeletalMesh
{
    float3 Position : POSITION;
    float4 Color : COLOR;
    float3 Normal : NORMAL;
    float4 Tangent : TANGENT;
    float2 UV : TEXCOORD;
    uint4 BoneIndices : BONE_INDICES;
    float4 BoneWeights : BONE_WEIGHTS;
};

struct VS_OUTPUT_CubeMap
{
    float4 position : POSITION;
};

StructuredBuffer<float4x4> BoneMatrices : register(t1);

cbuffer FCPUSkinningConstants : register(b2)
{
    bool bCPUSkinning;
    float3 pad0;
}

cbuffer DepthCubeMapConstants : register(b0)
{
    row_major matrix World;
    row_major matrix ViewProj[NUM_FACES];
}

VS_OUTPUT_CubeMap mainVS_SM(VS_INPUT_StaticMesh Input)
{
    VS_OUTPUT_CubeMap output;
    //output.position = mul(float4(Input.Position, 1.0f), World);
    output.position = float4(Input.Position, 1.0f);
    return output;
}

VS_OUTPUT_CubeMap mainVS_SKM(VS_INPUT_SkeletalMesh Input)
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
    
    VS_OUTPUT_CubeMap output;
    //output.position = mul(float4(Input.Position, 1.0f), World);
    output.position = SkinnedPosition;
    return output;
}
