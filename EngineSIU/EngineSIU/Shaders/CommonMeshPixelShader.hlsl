
#include "ShaderRegisters.hlsl"

cbuffer MaterialConstants : register(b1)
{
    FMaterial Material;
}

cbuffer FlagConstants : register(b2)
{
    bool IsLit;
    float3 flagPad0;
}

cbuffer SubMeshConstants : register(b3)
{
    bool IsSelectedSubMesh;
    float3 SubMeshPad0;
}

cbuffer TextureConstants : register(b4)
{
    float2 UVOffset;
    float2 TexturePad0;
}

cbuffer TileLightCullSettings : register(b8)
{
    uint2 ScreenSize; // 화면 해상도
    uint2 TileSize; // 한 타일의 크기 (예: 16x16)

    float NearZ; // 카메라 near plane
    float FarZ; // 카메라 far plane

    row_major matrix TileViewMatrix; // View 행렬
    row_major matrix TileProjectionMatrix; // Projection 행렬
    row_major matrix TileInverseProjection; // Projection^-1, 뷰스페이스 복원용

    uint NumLights; // 총 라이트 수
    uint Enable25DCulling; // 1이면 2.5D 컬링 사용
}

#include "Light.hlsl"

float4 mainPS(PS_INPUT_CommonMesh Input) : SV_Target
{
    float BaseAlpha = 1.0 - Material.Transparency;
    
    // Diffuse
    float3 DiffuseColor = Material.DiffuseColor;
    if (Material.TextureFlag & TEXTURE_FLAG_DIFFUSE)
    {
        float4 DiffuseColor4 = MaterialTextures[TEXTURE_SLOT_DIFFUSE].Sample(SamplerLinearWrap, Input.UV);
        DiffuseColor = DiffuseColor4.rgb;
        BaseAlpha = DiffuseColor4.a;
    }

    // Alpha
    if (Material.TextureFlag & TEXTURE_FLAG_ALPHA)
    {
        float Alpha = MaterialTextures[TEXTURE_SLOT_ALPHA].Sample(SamplerLinearWrap, Input.UV).r;
        BaseAlpha = Alpha;
    }
    
    // Normal
    float3 WorldNormal = normalize(Input.WorldNormal);
    if (Material.TextureFlag & TEXTURE_FLAG_NORMAL)
    {
        float3 Tangent = normalize(Input.WorldTangent.xyz);
        float Sign = Input.WorldTangent.w;
        float3 BiTangent = cross(WorldNormal, Tangent) * Sign;

        float3x3 TBN = float3x3(Tangent, BiTangent, WorldNormal);
        
        float3 Normal = MaterialTextures[TEXTURE_SLOT_NORMAL].Sample(SamplerLinearWrap, Input.UV).rgb;
        Normal = normalize(2.f * Normal - 1.f);
        WorldNormal = normalize(mul(Normal, TBN));
    }

#ifndef LIGHTING_MODEL_PBR
    // Specular Color
    float3 SpecularColor = Material.SpecularColor;
    if (Material.TextureFlag & TEXTURE_FLAG_SPECULAR)
    {
        SpecularColor = MaterialTextures[TEXTURE_SLOT_SPECULAR].Sample(SamplerLinearWrap, Input.UV).rgb;
    }

    // Specular Exponent or Glossiness
    float Shininess = Material.Shininess;
    if (Material.TextureFlag & TEXTURE_FLAG_SHININESS)
    {
        Shininess = MaterialTextures[TEXTURE_SLOT_SHININESS].Sample(SamplerLinearWrap, Input.UV).r;
        Shininess = 1000 * Shininess * Shininess; // y = 1000 * x ^ 2
    }
#endif

    // Emissive Color
    float3 EmissiveColor = Material.EmissiveColor;
    if (Material.TextureFlag & TEXTURE_FLAG_EMISSIVE)
    {
        EmissiveColor = MaterialTextures[TEXTURE_SLOT_EMISSIVE].Sample(SamplerLinearWrap, Input.UV).rgb;
    }
    EmissiveColor *= 5.0; // 5.0은 임의의 값

#ifdef LIGHTING_MODEL_PBR
    // Metallic
    float Metallic = Material.Metallic;
    if (Material.TextureFlag & TEXTURE_FLAG_METALLIC)
    {
        Metallic = MaterialTextures[TEXTURE_SLOT_METALLIC].Sample(SamplerLinearWrap, Input.UV).b;
    }

    // Roughness
    float Roughness = Material.Roughness;
    if (Material.TextureFlag & TEXTURE_FLAG_ROUGHNESS)
    {
        Roughness = MaterialTextures[TEXTURE_SLOT_ROUGHNESS].Sample(SamplerLinearWrap, Input.UV).g;
    }
#endif
    
    // Begin for Tile based light culled result
    // 현재 픽셀이 속한 타일 계산 (input.position = 화면 픽셀좌표계)
    uint2 PixelCoord = uint2(Input.Position.xy);
    uint2 TileCoord = PixelCoord / TileSize; // 각 성분별 나눔
    uint TilesX = ScreenSize.x / TileSize.x; // 한 행에 존재하는 타일 수
    uint FlatTileIndex = TileCoord.x + TileCoord.y * TilesX;
    // End for Tile based light culled result
    
    // Lighting
    float4 FinalPixelColor = float4(0.f, 0.f, 0.f, 1.f);
    if (IsLit)
    {
#ifdef LIGHTING_MODEL_GOURAUD
        FinalPixelColor = float4(Input.Color.rgb, BaseAlpha);
        // Gouraud 셰이딩은 Emissive 제외
#else
        float4 LitResult = Lighting(
            Input.WorldPosition,
            WorldNormal,
            ViewWorldLocation,
            DiffuseColor,
    #ifdef LIGHTING_MODEL_PBR
            Metallic,
            Roughness,
    #else
            SpecularColor,
            Shininess,
    #endif
            BaseAlpha,
            FlatTileIndex
        );

        // Apply Emissive
        float3 FinalRGB = LitResult.rgb + EmissiveColor;
        float FinalA = LitResult.a;
        
        FinalPixelColor = float4(FinalRGB, FinalA);
#endif
    }
    else
    {
        float3 UnlitRGB = DiffuseColor + EmissiveColor;

        FinalPixelColor = float4(UnlitRGB, BaseAlpha);
    }
    
    if (bIsSelected)
    {
        FinalPixelColor.rgb += float3(0.01, 0.01, 0.0);
    }

    return FinalPixelColor;
}
