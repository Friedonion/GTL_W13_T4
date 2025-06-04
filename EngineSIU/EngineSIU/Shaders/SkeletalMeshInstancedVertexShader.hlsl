 #define MAX_NUM_INSTANCE 128

cbuffer CameraBuffer : register(b13)
{
    row_major matrix ViewMatrix;
    row_major matrix InvViewMatrix;
    
    row_major matrix ProjectionMatrix;
    row_major matrix InvProjectionMatrix;
    
    float3 ViewWorldLocation; // TODO: 가능하면 버퍼에서 빼기
    float ViewPadding;
    
    float NearClip;
    float FarClip;
    float2 ProjectionPadding;
}

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

struct PS_INPUT_CommonMesh
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR;
    float2 UV : TEXCOORD0;
    float3 WorldNormal : TEXCOORD1;
    float4 WorldTangent : TEXCOORD2;
    float3 WorldPosition : TEXCOORD3;
};

struct ObjectMatrices
{
    row_major matrix WorldMatrix;
    row_major matrix InverseTransposedWorld;
};

cbuffer ObjectBuffer : register(b12)
{
    int NumBones;
    int3 Pad0;
    ObjectMatrices InstanceMatrices[MAX_NUM_INSTANCE]; // 실제론 InstanceID개수만큼 들어가있음
};


// InstanceID개수 * NumBones개수만큼 들어가있음
// (InstanceID, BoneID)라고 하면
// (0,0) (0,1) (0,2) ... (0,NumBones-1), (1,0), (1,1), (1,2) ... (InstanceID 개수 - 1, NumBones-1)
StructuredBuffer<float4x4> BoneMatricesInstanced : register(t1);

#ifdef LIGHTING_MODEL_GOURAUD
SamplerState DiffuseSampler : register(s0);

Texture2D DiffuseTexture : register(t0);

cbuffer MaterialConstants : register(b1)
{
    FMaterial Material;
}

#include "Light.hlsl"
#endif

PS_INPUT_CommonMesh mainVS(VS_INPUT_SkeletalMesh Input, uint InstanceID : SV_InstanceID)
{
    PS_INPUT_CommonMesh Output;

    float4 SkinnedPosition = float4(0, 0, 0, 0);
    float3 SkinnedNormal = float3(0, 0, 0);
    float3 SkinnedTangent = float3(0, 0, 0);
    
    uint BoneInstancingOffset = InstanceID * NumBones;
    
    // 가중치 합산
    float TotalWeight = 0.0f;
    
    for (int i = 0; i < 4; ++i)
    {
        float Weight = Input.BoneWeights[i];
        TotalWeight += Weight;
        
        if (Weight > 0.0f)
        {
            uint BoneIdx = Input.BoneIndices[i] + BoneInstancingOffset;
            
            // 본 행렬 적용 (BoneMatrices는 이미 최종 스키닝 행렬)
            // FBX SDK에서 가져온 역바인드 포즈 행렬이 이미 포함됨
            float4 pos = mul(float4(Input.Position, 1.0f), BoneMatricesInstanced[BoneIdx]);
            float3 norm = mul(float4(Input.Normal, 0.0f), BoneMatricesInstanced[BoneIdx]).xyz;
            float3 tan = mul(float4(Input.Tangent.xyz, 0.0f), BoneMatricesInstanced[BoneIdx]).xyz;
            
            SkinnedPosition += Weight * pos;
            SkinnedNormal += Weight * norm;
            SkinnedTangent += Weight * tan;
        }

        // 가중치 예외 처리
        if (TotalWeight < 0.001f)
        {
            SkinnedPosition = float4(Input.Position, 1.0f);
            SkinnedNormal = Input.Normal;
            SkinnedTangent = Input.Tangent.xyz;
        }
        else if (abs(TotalWeight - 1.0f) > 0.001f && TotalWeight > 0.001f)
        {
            // 가중치 합이 1이 아닌 경우 정규화
            SkinnedPosition /= TotalWeight;
            SkinnedNormal /= TotalWeight;
            SkinnedTangent /= TotalWeight;
        }
    }
    
    Output.Position = SkinnedPosition;
    Output.Position = mul(Output.Position, InstanceMatrices[InstanceID].WorldMatrix);
    Output.WorldPosition = Output.Position.xyz;
    
    Output.Position = mul(Output.Position, ViewMatrix);
    Output.Position = mul(Output.Position, ProjectionMatrix);
    
    Output.WorldNormal = normalize(mul(normalize(SkinnedNormal), (float3x3) InstanceMatrices[InstanceID].InverseTransposedWorld));

    // Begin Tangent
    float3 WorldTangent = mul(normalize(SkinnedTangent), (float3x3) InstanceMatrices[InstanceID].WorldMatrix);
    WorldTangent = normalize(WorldTangent - Output.WorldNormal * dot(Output.WorldNormal, WorldTangent));

    Output.WorldTangent = float4(WorldTangent, Input.Tangent.w);
    // End Tangent

    Output.UV = Input.UV;

#ifdef LIGHTING_MODEL_GOURAUD
    float3 DiffuseColor = Input.Color;
    if (Material.TextureFlag & TEXTURE_FLAG_DIFFUSE)
    {
        DiffuseColor = DiffuseTexture.SampleLevel(DiffuseSampler, Input.UV, 0).rgb;
    }
    float3 Diffuse = Lighting(
        Output.WorldPosition, Output.WorldNormal, ViewWorldLocation,
        DiffuseColor, Material.SpecularColor, Material.Shininess,
        1.0
    );
    Output.Color = float4(Diffuse.rgb, 1.0);
#else
    Output.Color = Input.Color;
#endif

    return Output;
}
