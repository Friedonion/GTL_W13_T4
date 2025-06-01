
#include "ShaderRegisters.hlsl"

cbuffer MaterialConstants : register(b0)
{
    FMaterial Material;
}

cbuffer SubUVConstant : register(b1)
{
    float2 UVOffset;
    float2 UVScale; // sub UV 셀의 크기 (예: 1/CellsPerColumn, 1/CellsPerRow)
}

struct PS_Input
{
    float4 Position : SV_Position;
    float2 UV : TEXCOORD0;
    float4 Color : COLOR0;
    float RelativeTime : TEXCOORD1;
    float ParticleId : TEXCOORD2;
    int SubImageIndex : TEXCOORD3;
};

float4 main(PS_Input Input) : SV_TARGET
{
    float4 FinalColor = float4(0.f, 0.f, 0.f, 1.f);

    const float UVScale_Horizontal = UVScale.x;
    const float UVScale_Vertical = UVScale.y;
    const float UVNum_Horizontal = 1.0f / UVScale_Horizontal;
    const float UVNum_Vertical = 1.0f / UVScale_Vertical;
    
    const float TileColumn = fmod(Input.SubImageIndex, UVNum_Horizontal);
    const float TileRow = floor(Input.SubImageIndex / UVNum_Horizontal);

    const float2 SubUVOffset = float2(TileColumn, TileRow) * UVScale;
    
    float2 UV = Input.UV * UVScale + SubUVOffset;

    float4 Color = Input.Color;
    if (Material.TextureFlag & TEXTURE_FLAG_DIFFUSE)
    {
        Color *= MaterialTextures[TEXTURE_SLOT_DIFFUSE].Sample(SamplerLinearWrap, UV);        
    }

    FinalColor = Color;
    
    return FinalColor;
}
