
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

float4 main(PS_INPUT_CommonMesh Input) : SV_TARGET
{
    float4 FinalColor = float4(0.f, 0.f, 0.f, 1.f);
    
    float2 UV = Input.UV * UVScale + UVOffset;

    float4 Color = Input.Color;
    if (Material.TextureFlag & TEXTURE_FLAG_DIFFUSE)
    {
        Color *= MaterialTextures[TEXTURE_SLOT_DIFFUSE].Sample(SamplerLinearWrap, UV);        
    }

    FinalColor = Color;
    
    return FinalColor;
}
