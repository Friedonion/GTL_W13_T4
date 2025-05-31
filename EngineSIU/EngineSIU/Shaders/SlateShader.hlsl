
Texture2D ViewportTexture : register(t120);

SamplerState Sampler : register(s3); // Point Clamp

cbuffer SlateTransform : register(b11)
{
    float2 SlateScale;
    float2 SlateOffset;
}

struct PS_Input
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD;
};

PS_Input mainVS(uint VertexID : SV_VertexID)
{
    PS_Input Output;

    float2 ScreenPositions[6] = {
        float2(-1,  1),  // Top Left
        float2(1,  1),  // Top Right
        float2(-1, -1),  // Bottom Left
        float2(1,  1),  // Top Right
        float2(1, -1),  // Bottom Right
        float2(-1, -1)   // Bottom Left
    };

    float2 UVs[6] = {
        float2(0, 0), float2(1, 0), float2(0, 1),
        float2(1, 0), float2(1, 1), float2(0, 1)
    };

    float2 SlatePosition = ScreenPositions[VertexID];
    SlatePosition = SlatePosition * SlateScale + SlateOffset;

    Output.Position = float4(SlatePosition, 0, 1);
    Output.UV = UVs[VertexID];

    return Output;
}

float4 mainPS(PS_Input Input) : SV_TARGET
{
    float4 Scene = ViewportTexture.Sample(Sampler, Input.UV);

    return Scene;
}
