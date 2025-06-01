#define MAX_CASCADE_NUM 5 // TO DO : TO FIX!!!!
#define NUM_CASCADES 3

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

StructuredBuffer<float4x4> BoneMatrices : register(t1);

cbuffer FCPUSkinningConstants : register(b2)
{
    bool bCPUSkinning;
    float3 pad0;
}

cbuffer CascadeConstantBuffer : register(b0)
{
    row_major matrix World;
    row_major matrix CascadedViewProj[MAX_CASCADE_NUM];
};

struct GS_INPUT
{
    float4 position : SV_POSITION;
};

struct GS_OUTPUT
{
    float4 pos : SV_POSITION;
    uint RTIndex : SV_RenderTargetArrayIndex;
};

GS_INPUT mainVS_SM(VS_INPUT_StaticMesh Input)
{
    GS_INPUT output;
    float4 pos = mul(float4(Input.Position, 1.0f), World);
    output.position = pos;
    return output;
}

GS_INPUT mainVS_SKM(VS_INPUT_SkeletalMesh Input)
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
    
    GS_INPUT output;
    float4 pos = mul(SkinnedPosition, World);
    output.position = pos;
    return output;
}

[maxvertexcount(3 * NUM_CASCADES)]
void mainGS( 
    triangle GS_INPUT input[3], 
    inout TriangleStream<GS_OUTPUT> TriStream
)
{
    for (uint csmIdx = 0; csmIdx < NUM_CASCADES; ++csmIdx)
    {
        for (int i = 0; i < 3; ++i)
        {
            GS_OUTPUT output;
            float4 worldPos = input[i].position;
            //worldPos = mul(input[i].position, World);
            output.pos = mul(worldPos, CascadedViewProj[csmIdx]);
            output.RTIndex = csmIdx;

            TriStream.Append(output);
        }
        TriStream.RestartStrip();
    }
        
}

float4 mainPS(GS_OUTPUT input) : SV_TARGET
{
    float depth = input.pos.z / input.pos.w;
    return float4(depth, depth, depth, 1.0f);
}
