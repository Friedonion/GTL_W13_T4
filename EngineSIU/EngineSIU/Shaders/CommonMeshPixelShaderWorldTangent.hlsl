
#include "ShaderRegisters.hlsl"

float4 mainPS(PS_INPUT_CommonMesh Input) : SV_Target
{
    float3 FinalColor = normalize(Input.WorldTangent.xyz);
    
    FinalColor = (FinalColor + 1.f) / 2.f;
    
    return float4(FinalColor, 1.f);
}
