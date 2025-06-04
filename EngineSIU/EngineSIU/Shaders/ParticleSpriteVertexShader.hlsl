
#include "ShaderRegisters.hlsl"

struct FParticleSpriteVertex
{
    float3 Position;
    float RelativeTime;
    float3 OldPosition;
    float ParticleId;
    float2 Size;
    float Rotation;
    float SubImageIndex;
    float4 Color;
};

StructuredBuffer<FParticleSpriteVertex> InstanceBuffer : register(t1);

struct VS_Input
{
    uint VertexID : SV_VertexID;
    uint InstanceID : SV_InstanceID;
};

struct PS_Input
{
    float4 Position : SV_Position;
    float2 UV : TEXCOORD0;
    float4 Color : COLOR0;
    float RelativeTime : TEXCOORD1;
    float ParticleId : TEXCOORD2;
    int SubImageIndex : TEXCOORD3;
};

PS_Input main(VS_Input Input)
{
    // P0: Bottom-Left  (-0.5, -0.5) UV(0,1)
    // P1: Bottom-Right ( 0.5, -0.5) UV(1,1)
    // P2: Top-Left     (-0.5,  0.5) UV(0,0)
    // P3: Top-Right    ( 0.5,  0.5) UV(1,0)
    float2 LocalOffsets[4] =
    {
        float2(-0.5f, -0.5f), // P0
        float2(0.5f, -0.5f), // P1
        float2(-0.5f, 0.5f), // P2
        float2(0.5f, 0.5f) // P3
    };

    float2 TexCoords[4] =
    {
        float2(0.0f, 1.0f), // P0 (텍스처의 좌하단)
        float2(1.0f, 1.0f), // P1 (텍스처의 우하단)
        float2(0.0f, 0.0f), // P2 (텍스처의 좌상단)
        float2(1.0f, 0.0f) // P3 (텍스처의 우상단)
    };

    uint CornerIndices[6] =
    {
        0, // VertexID 0 -> P0 (Bottom-Left)
        2, // VertexID 1 -> P2 (Top-Left)
        1, // VertexID 2 -> P1 (Bottom-Right)  // Triangle 1: P0-P2-P1 (CW)

        1, // VertexID 3 -> P1 (Bottom-Right)
        2, // VertexID 4 -> P2 (Top-Left)      // Triangle 2: P1-P2-P3 (CW)
        3 // VertexID 5 -> P3 (Top-Right)
    };

    FParticleSpriteVertex Particle = InstanceBuffer[Input.InstanceID];
    
    PS_Input Output = (PS_Input) 0;
    
    uint CornerIdx = CornerIndices[Input.VertexID];
    float2 CornerOffset = LocalOffsets[CornerIdx];
    Output.UV = TexCoords[CornerIdx];

    // 3. 파티클 크기 적용
    CornerOffset *= Particle.Size; // Size.x는 너비, Size.y는 높이

    // 4. 롤(roll) 회전 적용 (뷰 평면에서의 2D 회전)
    //    particle.Rotation은 라디안 값이어야 합니다.
    float s = sin(Particle.Rotation);
    float c = cos(Particle.Rotation);

    float2 RotatedOffset;
    RotatedOffset.x = CornerOffset.x * c - CornerOffset.y * s;
    RotatedOffset.y = CornerOffset.x * s + CornerOffset.y * c;

    // 5. 빌보딩(Billboarding) 및 최종 버텍스 위치 계산
    //    파티클의 월드 중심 위치를 뷰 공간으로 변환합니다.
    float4 ParticleCenter_WS = mul(float4(Particle.Position, 1.0f), WorldMatrix);
    float4 ParticleCenter_VS = mul(ParticleCenter_WS, ViewMatrix);

    //    뷰 공간에서 2D 오프셋(회전 및 크기 적용됨)을 더합니다.
    //    Z값은 파티클 중심의 Z값을 그대로 사용하므로, 쿼드는 카메라를 향하게 됩니다.
    float4 VertexPosition_VS = ParticleCenter_VS + float4(RotatedOffset, 0.0f, 0.0f);

    // 6. 뷰 공간 위치를 클립 공간으로 변환
    Output.Position = mul(VertexPosition_VS, ProjectionMatrix);

    // 7. 나머지 파티클 데이터를 픽셀 셰이더로 전달
    Output.Color = Particle.Color;
    Output.RelativeTime = Particle.RelativeTime;
    Output.ParticleId = Particle.ParticleId;
    Output.SubImageIndex = Particle.SubImageIndex;

    return Output;
}
