
#define MAX_LIGHTS 16 

#define MAX_DIRECTIONAL_LIGHT 16
#define MAX_POINT_LIGHT 16
#define MAX_SPOT_LIGHT 16
#define MAX_AMBIENT_LIGHT 16

#define POINT_LIGHT         1
#define SPOT_LIGHT          2
#define DIRECTIONAL_LIGHT   3
#define AMBIENT_LIGHT       4

#define MAX_LIGHT_PER_TILE 1024

#define NEAR_PLANE 0.1
#define LIGHT_RADIUS_WORLD 7

#define MAX_CASCADE_NUM 5

struct FAmbientLightInfo
{
    float4 AmbientColor;
};

struct FDirectionalLightInfo
{
    float4 LightColor;

    float3 Direction;
    float Intensity;
    
    row_major matrix LightViewProj;
    row_major matrix LightInvProj; // 섀도우맵 생성 시 사용한 VP 행렬
    
    uint ShadowMapArrayIndex;//캐스캐이드전 임시 배열
    uint  CastShadows;
    float ShadowBias;
    float Padding3; // 필요시

    float OrthoWidth;
    // 직교 투영 볼륨의 월드 단위 높이 (섀도우 영역)
    float OrthoHeight;
    // 섀도우 계산을 위한 라이트 시점의 Near Plane (음수 가능)
    float ShadowNearPlane;
    // 섀도우 계산을 위한 라이트 시점의 Far Plane
    float ShadowFarPlane;
};

struct FPointLightInfo
{
    float4 LightColor;

    float3 Position;
    float Radius;

    int Type;
    float Intensity;
    float Attenuation;
    float Padding;

    // --- Shadow Info ---
    row_major matrix LightViewProj[6]; // 섀도우맵 생성 시 사용한 VP 행렬
    
    uint CastShadows;
    float ShadowBias;
    uint ShadowMapArrayIndex; // 필요시
    float Padding2; // 필요시
};

struct FSpotLightInfo
{
    float4 LightColor;

    float3 Position;
    float Radius;

    float3 Direction;
    float Intensity;

    int Type;
    float InnerRad;
    float OuterRad;
    float Attenuation;
    
    // --- Shadow Info ---
    row_major matrix LightViewProj; // 섀도우맵 생성 시 사용한 VP 행렬
    
    uint CastShadows;
    float ShadowBias;
    uint ShadowMapArrayIndex; // 필요시
    float Padding2; // 필요시
};

cbuffer FLightInfoBuffer : register(b0)
{
    FAmbientLightInfo Ambient[MAX_AMBIENT_LIGHT];
    FDirectionalLightInfo Directional[MAX_DIRECTIONAL_LIGHT];
    FPointLightInfo PointLights[MAX_POINT_LIGHT]; //삭제 예정
    FSpotLightInfo SpotLights[MAX_SPOT_LIGHT]; // 삭제 예정
    
    int DirectionalLightsCount;
    int PointLightsCount;
    int SpotLightsCount;
    int AmbientLightsCount;
};

cbuffer ShadowFlagConstants : register(b5)
{
    uint IsShadow;
    float3 shadowFlagPad0;
}

cbuffer CascadeConstantBuffer : register(b9)
{
    row_major matrix World;
    row_major matrix CascadedViewProj[MAX_CASCADE_NUM];
    row_major matrix CascadedInvViewProj[MAX_CASCADE_NUM];
    row_major matrix CascadedInvProj[MAX_CASCADE_NUM];
    float4 CascadeSplits;
    float2 cascadepad;
};

struct LightPerTiles
{
    uint NumLights;
    uint Indices[MAX_LIGHT_PER_TILE];
    uint Padding[3];
};

// 인덱스별 색 지정 함수
float4 DebugCSMColor(uint idx)
{
    if (idx == 0)
    {
        return float4(1, 0, 0, 1); // 빨강
    }
    if (idx == 1)
    {
        return float4(0, 1, 0, 1); // 초록
    }
    if (idx == 2)
    {
        return float4(0, 0, 1, 1); // 파랑
    }
    return float4(1, 1, 1, 1); // 나머지 – 흰색
}

StructuredBuffer<FPointLightInfo> gPointLights : register(t10);
StructuredBuffer<FSpotLightInfo> gSpotLights   : register(t11);

StructuredBuffer<uint> PerTilePointLightIndexBuffer : register(t12);
StructuredBuffer<uint> PerTileSpotLightIndexBuffer  : register(t13);

// Begin Shadow
SamplerComparisonState ShadowSamplerCmp : register(s10);
SamplerState ShadowPointSampler : register(s11);

Texture2DArray SpotShadowMapArray : register(t50);
Texture2DArray DirectionShadowMapArray : register(t51);
TextureCubeArray PointShadowMapArray : register(t52);

uint GetCascadeIndex(float ViewDepth)
{
    // viewDepth 는 LightSpace 깊이(z) 또는 NDC 깊이 복원 뷰 깊이

    for (uint i = 0; i < MAX_CASCADE_NUM; ++i)
    {
        // splits 배열에는 [0]=near, [N]=far 까지 로그 스플릿 저장됨 ex)0..2..4..46..1000
        if (ViewDepth <= CascadeSplits[i + 1])
        {
            return i;
        }
    }
    return MAX_CASCADE_NUM - 1;
}

// NdcDepthToViewDepth
float N2V(float ndcDepth, matrix invProj)
{
    float4 pointView = mul(float4(0, 0, ndcDepth, 1), invProj);
    return pointView.z / pointView.w;
}

float PCF_Filter(float2 uv, float zReceiverNdc, float filterRadiusUV, uint csmIndex)
{
    float sum = 0.0f;
    [unroll]
    for (int i = 0; i < 64; ++i)
    {
        // TODO (offset, 0)에 배열(slice) 인덱스 넣기
        float2 offset = diskSamples64[i] * filterRadiusUV;
        sum += DirectionShadowMapArray.SampleCmpLevelZero(ShadowSamplerCmp, float3(uv + offset, csmIndex), zReceiverNdc);
    }
    return sum / 64;
}

void FindBlocker(out float avgBlockerDepthView, out float numBlockers, float2 uv,
                 float zReceiverView, Texture2DArray DirectionShadowMapArray, matrix InvProj, float LightRadiusWorld, uint csmIndex, FDirectionalLightInfo LightInfo)
{
    float LightRadiusUV = LightRadiusWorld / LightInfo.OrthoWidth; // TO FIX!
    float searchRadius = LightRadiusUV * (zReceiverView - NEAR_PLANE) / zReceiverView; // TO FIX! NearPlane
    float blockerSum = 0;
    numBlockers = 0;

    for (int i = 0; i < 64; ++i)
    {
        // TODO : slice index 받아야 함 (searchRadius , 배열 인덱스)
        float ShadowMapDepth = DirectionShadowMapArray.SampleLevel(ShadowPointSampler, float3(uv + diskSamples64[i] * searchRadius, csmIndex), 0).r;
        ShadowMapDepth = N2V(ShadowMapDepth, InvProj);
        if (ShadowMapDepth < zReceiverView)
        {
            blockerSum += ShadowMapDepth;
            numBlockers += 1;
        }
    }
    avgBlockerDepthView = (numBlockers > 0) ? (blockerSum / numBlockers) : 0.0f;
}

// TODO: 사용하려면 계산식을 수정할 필요가 있음.
float PCSS(float2 UV, float ZReceiverNDC, Texture2DArray DirectionShadowMapArray, matrix ShadowInvProj, float LightRadiusWorld, uint CsmIndex, FDirectionalLightInfo LightInfo)
{
    float LightRadiusUV = LightRadiusWorld / 2.0; // TO FIX!
    float ZReceiverView = N2V(ZReceiverNDC, CascadedInvProj[CsmIndex]);


    // 1. Blocker Search
    float AvgBlockerDepthView = 0;
    float NumBlockers = 0;

    FindBlocker(AvgBlockerDepthView, NumBlockers, UV, ZReceiverView, DirectionShadowMapArray, CascadedInvProj[CsmIndex], LightRadiusWorld, CsmIndex, LightInfo);


    if (NumBlockers < 1)
    {
        // There are no Occluders so early out(this saves filtering)
        return 1.0f;
    }
    else
    {
        // 2. Penumbra Size
        float PenumbraRatio = (ZReceiverView - AvgBlockerDepthView) / AvgBlockerDepthView;
        float FilterRadiusUV = PenumbraRatio * LightRadiusUV * NEAR_PLANE / ZReceiverView; // TO FIX!!!!

        // 3. Filtering
        return PCF_Filter(UV, ZReceiverNDC, FilterRadiusUV, CsmIndex);
    }
}

float CalculateDirectionalShadowFactor(float3 WorldPosition, float3 WorldNormal, FDirectionalLightInfo LightInfo, // 라이트 정보 전체 전달
                                Texture2DArray DirectionShadowMapArray,
                                SamplerComparisonState ShadowSampler)
{
    if (LightInfo.CastShadows == 0)
    {
        return 1.0f;
    }
    
    float ShadowFactor = 1.0;
    float NdotL = dot(normalize(WorldNormal), LightInfo.Direction);
    float bias = LightInfo.ShadowBias;
    
    // 1. Project World Position to Light Screen Space
    float4 LightScreen = mul(float4(WorldPosition, 1.0f), LightInfo.LightViewProj);
    LightScreen.xyz /= LightScreen.w; // Perspective Divide -> [-1, 1] 범위로 변환

    // 2. 광원 입장의 Texture 좌표계로 변환
    float2 ShadowMapTexCoord = { LightScreen.x, -LightScreen.y }; // NDC 좌표계와 UV 좌표계는 Y축 방향이 반대
    ShadowMapTexCoord += 1.0;
    ShadowMapTexCoord *= 0.5;

    float LightDistance = LightScreen.z;
    LightDistance -= bias;


    float4 posCam = mul(float4(WorldPosition, 1), ViewMatrix);
    float depthCam = posCam.z; 
    uint CsmIndex = GetCascadeIndex(depthCam);
    //CsmIndex = 1;
    float4 posLS = mul(float4(WorldPosition, 1), CascadedViewProj[CsmIndex]);
    float2 uv = posLS.xy * 0.5f + 0.5f;
    uv.y = 1 - uv.y;
    float zReceiverNdc = posLS.z -= bias;
    ShadowFactor = DirectionShadowMapArray.SampleCmpLevelZero(ShadowSamplerCmp, float3(uv, CsmIndex), zReceiverNdc);

    //ShadowFactor = PCSS(uv, zReceiverNdc, DirectionShadowMapArray, CascadedInvViewProj[CsmIndex], LIGHT_RADIUS_WORLD, CsmIndex, LightInfo);
    return ShadowFactor;
}

int GetMajorFaceIndex(float3 Dir)
{
    float3 absDir = abs(Dir);
    if (absDir.x > absDir.y && absDir.x > absDir.z)
    {
        return Dir.x > 0.0f ? 0 : 1;
    }
    if (absDir.y > absDir.z)
    {
        return Dir.y > 0.0f ? 2 : 3;
    }
    return Dir.z > 0.0f ? 4 : 5;
}

float CalculatePointShadowFactor(float3 WorldPosition, FPointLightInfo LightInfo, // 라이트 정보 전체 전달
                                TextureCubeArray ShadowMapArray,
                                SamplerComparisonState ShadowSampler)
{
    // 1) 광원→조각 방향 (큐브맵 샘플링 좌표)
    float3 Dir = normalize(WorldPosition - LightInfo.Position);
    // 2) 해당 face의 뷰·프로젝션 적용
    int face = GetMajorFaceIndex(Dir);
    float4 posCS = mul(float4(WorldPosition, 1.0f), LightInfo.LightViewProj[face]);
    // 3) 클립스페이스 깊이
    float refDepth = posCS.z / posCS.w;
    // 5) 하드웨어 비교 샘플
    float3 SampleDir = normalize(WorldPosition - LightInfo.Position);
    float shadow = ShadowMapArray.SampleCmpLevelZero(ShadowSampler, float4(SampleDir, (float)LightInfo.ShadowMapArrayIndex), refDepth - LightInfo.ShadowBias).r;
    return shadow;
}

// 기본적인 그림자 계산 함수 (Directional/Spot 용)
// 하드웨어 PCF (SamplerComparisonState 사용) 예시
float CalculateSpotShadowFactor(float3 WorldPosition, FSpotLightInfo LightInfo, // 라이트 정보 전체 전달
                                Texture2DArray ShadowMapArray,
                                SamplerComparisonState ShadowSampler)
{
    // if (!LightInfo.CastShadows)
    // {
    //     return 1.0f; // 그림자 안 드리움
    // }

    // 1 & 2. 라이트 클립 공간 좌표 계산
    float4 PixelPosLightClip = mul(float4(WorldPosition, 1.0f), LightInfo.LightViewProj);

    // 3. 섀도우 맵 UV 계산 [0, 1]
    float2 ShadowMapUV = PixelPosLightClip.xy / PixelPosLightClip.w;
    ShadowMapUV = ShadowMapUV * float2(0.5, -0.5) + 0.5; // Y 반전 필요시

    // 4. 현재 깊이 계산
    float CurrentDepth = PixelPosLightClip.z / PixelPosLightClip.w;

    // UV 범위 체크 (라이트 범위 밖)
    if (any(ShadowMapUV < 0.0f) || any(ShadowMapUV > 1.0f))
    {
        return 1.0f;
    }

    // 5 & 6. 배열의 특정 슬라이스 샘플링 및 비교
    float ShadowFactor = ShadowMapArray.SampleCmpLevelZero(
        ShadowSampler,
        float3(ShadowMapUV, (float)LightInfo.ShadowMapArrayIndex), // UV와 배열 인덱스 사용
        CurrentDepth - LightInfo.ShadowBias  // 바이어스 적용
    );

    return ShadowFactor;
}
// End Shadow

float GetDistanceAttenuation(float Distance, float Radius)
{
    float  InvRadius = 1.0 / Radius;
    float  DistSqr = Distance * Distance;
    float  RadiusMask = saturate(1.0 - DistSqr * InvRadius * InvRadius);
    RadiusMask *= RadiusMask;
    
    return RadiusMask / (DistSqr + 1.0);
}

float GetSpotLightAttenuation(float Distance, float Radius, float3 LightDir, float3 SpotDir, float InnerRadius, float OuterRadius)
{
    float DistAtten = GetDistanceAttenuation(Distance, Radius);
    
    float  CosTheta = dot(SpotDir, -LightDir);
    float  SpotMask = saturate((CosTheta - cos(OuterRadius)) / (cos(InnerRadius) - cos(OuterRadius)));
    SpotMask *= SpotMask;
    
    return DistAtten * SpotMask;
}

////////
/// Diffuse
////////
#define PI 3.14159265359

#ifdef LIGHTING_MODEL_PBR
inline float SchlickWeight(float CosTheta) // (1‑x)^5 == pow(1-x, 5)
{
    float m = saturate(1.0 - CosTheta);
    return m * m * m * m * m;
}

float DisneyDiffuse(float3 N, float3 L, float3 V, float Roughness)
{
    float3 H = normalize(L + V);
    float  NdotL = saturate(dot(N, L));
    float  NdotV = saturate(dot(N, V));
    float  LdotH = saturate(dot(L, H));
    float  LdotH2 = LdotH * LdotH;

    float  Fd90 = 0.5 + 2.0 * Roughness * LdotH2; // grazing boost

    float DiffuseFresnelL = SchlickWeight(LdotH);
    float DiffuseFresnelV = SchlickWeight(NdotV);

    float  Fd = (1.0 + (Fd90 - 1.0) * DiffuseFresnelL) * (1.0 + (Fd90 - 1.0) * DiffuseFresnelV);

    return Fd / PI;
}
#endif


////////
/// Specular
////////
float3 F_Schlick(float3 F0, float LdotH)
{
    return F0 + (1.0 - F0) * pow(1.0 - LdotH, 5.0);
}

#ifdef LIGHTING_MODEL_PBR
float  D_GGX(float NdotH, float alpha)
{
    float a2 = alpha * alpha;
    float d = (NdotH * NdotH) * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d);                 // Trowbridge‑Reitz
}

float  G_Smith(float NdotV, float NdotL, float alpha)
{
    float k = alpha * 0.5 + 0.0001;           // Schlick‑GGX (≈α/2)
    float gV = NdotV / (NdotV * (1.0 - k) + k);
    float gL = NdotL / (NdotL * (1.0 - k) + k);
    return gV * gL;
}

float3 CookTorranceSpecular(
    float3  F0,        // base reflectance (metallic → albedo)
    float   Roughness,
    float3  N, float3  V, float3  L
)
{
    float3  H = normalize(V + L);
    float   NdotL = saturate(dot(N, L));
    float   NdotV = saturate(dot(N, V));
    float   NdotH = saturate(dot(N, H));
    float   LdotH = saturate(dot(L, H));

    float   alpha = max(0.001, Roughness * Roughness);

    float   D = D_GGX(NdotH, alpha);
    float   G = G_Smith(NdotV, NdotL, alpha);
    float3  F = F_Schlick(F0, LdotH);

    return (D * G * F) / (4.0 * NdotV * NdotL + 1e-5);
}
#else
float BlinnPhongSpecular(float3 N, float3 L, float3 V, float Shininess, float SpecularStrength = 1.0)
{
    float3 H = normalize(L + V);
    float NdotH = saturate(dot(N, H));
    return pow(NdotH, Shininess) * SpecularStrength;
}
#endif

////////
/// BRDF
////////
struct FBRDFResult
{
    float3 DiffuseContribution;
    float3 SpecularContribution;
};

FBRDFResult CalculateBRDF(float3 L, float3 V, float3 N,
#ifdef LIGHTING_MODEL_PBR
    float3 BaseColor, float Metallic, float Roughness
#else
    float3 DiffuseColor, float3 SpecularColor, float Shininess
#endif
)
{
    FBRDFResult Result = (FBRDFResult)0;

#ifdef LIGHTING_MODEL_PBR
    float3 F0 = lerp(0.04, BaseColor, Metallic);

    float KdScale = 1.0 - Metallic;
    float3 Kd = BaseColor * KdScale;

    Result.DiffuseContribution = DisneyDiffuse(N, L, V, Roughness) * Kd;
    Result.SpecularContribution = CookTorranceSpecular(F0, Roughness, N, V, L);
#else
    Result.DiffuseContribution = DiffuseColor / PI; // BPR의 에너지 보존 결과와 유사한 밝기를 맞추기 위함
#ifdef LIGHTING_MODEL_BLINN_PHONG
    Result.SpecularContribution = BlinnPhongSpecular(N, L, V, Shininess) * SpecularColor;
#endif
#endif
    
    return Result;
}

////////
/// Calculate Light
////////
struct FLightOutput
{
    float3 DiffuseContribution;
    float3 SpecularContribution;
};

FLightOutput PointLight(int Index, float3 WorldPosition, float3 WorldNormal, float3 WorldViewPosition,
#ifdef LIGHTING_MODEL_PBR
    float3 BaseColor, float Metallic, float Roughness
#else
    float3 DiffuseColor, float3 SpecularColor, float Shininess
#endif
)
{
    FLightOutput Output = (FLightOutput)0;
    
    FPointLightInfo LightInfo = gPointLights[Index];
    
    float3 ToLight = LightInfo.Position - WorldPosition;
    float Distance = length(ToLight);
    
    float Attenuation = GetDistanceAttenuation(Distance, LightInfo.Radius);
    if (Attenuation <= 0.0)
    {
        return Output;
    }
    
    // --- 그림자 계산
    float Shadow = 1.0;
    if (LightInfo.CastShadows && IsShadow)
    {
        // 그림자 계산
        Shadow = CalculatePointShadowFactor(WorldPosition, LightInfo, PointShadowMapArray, ShadowSamplerCmp);
        // 그림자 계수가 0 이하면 더 이상 계산 불필요
        if (Shadow <= 0.0)
        {
            return Output;
        }
    }
    
    float3 L = normalize(ToLight);
    float NdotL = saturate(dot(WorldNormal, L));
    if (NdotL <= 0.0)
    {
        return Output;
    }
    
    float3 V = normalize(WorldViewPosition - WorldPosition);

    FBRDFResult BRDF = CalculateBRDF(L, V, WorldNormal,
#ifdef LIGHTING_MODEL_PBR
        BaseColor, Metallic, Roughness
#else
        DiffuseColor, SpecularColor, Shininess
#endif
    );

    float3 LightEnergy = LightInfo.LightColor.rgb * LightInfo.Intensity;

    Output.DiffuseContribution  = BRDF.DiffuseContribution * LightEnergy * Attenuation * Shadow * NdotL;
    Output.SpecularContribution = BRDF.SpecularContribution * LightEnergy * Attenuation * Shadow * NdotL;

    return Output;
}

FLightOutput SpotLight(int Index, float3 WorldPosition, float3 WorldNormal, float3 WorldViewPosition,
#ifdef LIGHTING_MODEL_PBR
    float3 BaseColor, float Metallic, float Roughness
#else
    float3 DiffuseColor, float3 SpecularColor, float Shininess
#endif
)
{
    FLightOutput Output = (FLightOutput)0;
    
    FSpotLightInfo LightInfo = gSpotLights[Index];
    
    float3 ToLight = LightInfo.Position - WorldPosition;
    float Distance = length(ToLight);
    float3 LightDir = normalize(ToLight);
    
    float SpotlightFactor = GetSpotLightAttenuation(Distance, LightInfo.Radius, LightDir, normalize(LightInfo.Direction), LightInfo.InnerRad, LightInfo.OuterRad);
    if (SpotlightFactor <= 0.0)
    {
        return Output;
    }

    // --- 그림자 계산
    float Shadow = 1.0;
    if (LightInfo.CastShadows && IsShadow)
    {
        // 그림자 계산
        Shadow  = CalculateSpotShadowFactor(WorldPosition, LightInfo, SpotShadowMapArray, ShadowSamplerCmp);
        // 그림자 계수가 0 이하면 더 이상 계산 불필요
        if (Shadow <= 0.0)
        {
            return Output;
        }
    }

    float3 L = normalize(ToLight);
    float NdotL = saturate(dot(WorldNormal, L));
    if (NdotL <= 0.0)
    {
        return Output;
    }
    
    float3 V = normalize(WorldViewPosition - WorldPosition);
    
    FBRDFResult BRDF = CalculateBRDF(L, V, WorldNormal,
#ifdef LIGHTING_MODEL_PBR
        BaseColor, Metallic, Roughness
#else
        DiffuseColor, SpecularColor, Shininess
#endif
    );
    
    float3 LightEnergy = LightInfo.LightColor.rgb * LightInfo.Intensity;

    Output.DiffuseContribution  = BRDF.DiffuseContribution * LightEnergy * SpotlightFactor * Shadow * NdotL;
    Output.SpecularContribution = BRDF.SpecularContribution * LightEnergy * SpotlightFactor * Shadow * NdotL;

    return Output;
}

FLightOutput DirectionalLight(int Index, float3 WorldPosition, float3 WorldNormal, float3 WorldViewPosition,
#ifdef LIGHTING_MODEL_PBR
    float3 BaseColor, float Metallic, float Roughness
#else
    float3 DiffuseColor, float3 SpecularColor, float Shininess
#endif
)
{
    FLightOutput Output = (FLightOutput)0;
    
    FDirectionalLightInfo LightInfo = Directional[Index];
    
    float4 posCam = mul(float4(WorldPosition, 1), ViewMatrix);
    float depthCam = posCam.z / posCam.w;
    uint csmIndex = GetCascadeIndex(depthCam); // 시각적 디버깅용
    
    // --- 그림자 계산
    float Shadow = 1.0;
    if (IsShadow)
    {
        Shadow = CalculateDirectionalShadowFactor(WorldPosition, WorldNormal, LightInfo, DirectionShadowMapArray, ShadowSamplerCmp);
        // 그림자 계수가 0 이하면 더 이상 계산 불필요
        if (Shadow <= 0.0)
        {
            return Output;
        }
    }

    float3 L = normalize(-LightInfo.Direction);
    float NdotL = saturate(dot(WorldNormal, L));
    if (NdotL <= 0.0)
    {
        return Output;
    }
    
    float3 V = normalize(WorldViewPosition - WorldPosition);
    
    FBRDFResult BRDF = CalculateBRDF(L, V, WorldNormal,
#ifdef LIGHTING_MODEL_PBR
        BaseColor, Metallic, Roughness
#else
        DiffuseColor, SpecularColor, Shininess
#endif
    );

    float3 LightEnergy = LightInfo.LightColor.rgb * LightInfo.Intensity;

    Output.DiffuseContribution  = BRDF.DiffuseContribution * LightEnergy * Shadow * NdotL /* DebugCSMColor(csmIndex) */;
    Output.SpecularContribution = BRDF.SpecularContribution * LightEnergy * Shadow * NdotL;

    return Output;
}


float GetFinalAlpha(float BaseAlpha, float SpecularLuminance, float3 V, float3 N,
#ifdef LIGHTING_MODEL_PBR
    float3 BaseColor, float Metallic
#else
    float3 SpecularColor
#endif
)
{
    float3 F0_View = float3(0.0, 0.0, 0.0);
#ifdef LIGHTING_MODEL_PBR
    F0_View = lerp(0.04, BaseColor, Metallic);
#else
    F0_View = SpecularColor; // Phong에서는 Specular 색상을 F0로 사용하거나 고정값 사용. 또는 float3(0.04, 0.04, 0.04);
#endif
    float ViewAngleFresnel = F_Schlick(F0_View, saturate(dot(N, V))).r;

    float HighlightOpacityContribution = saturate(SpecularLuminance);
    float TotalReflectanceInfluence = max(ViewAngleFresnel, HighlightOpacityContribution);

    float FinalAlpha = lerp(BaseAlpha, 1.0f, TotalReflectanceInfluence);
    
    return saturate(FinalAlpha); // [0, 1]
}


float4 Lighting(float3 WorldPosition, float3 WorldNormal, float3 WorldViewPosition,
#ifdef LIGHTING_MODEL_PBR
    float3 BaseColor, float Metallic, float Roughness,
#else
    float3 DiffuseColor, float3 SpecularColor, float Shininess,
#endif
    float BaseAlpha,
    uint TileIndex
)
{
    /**
     * RGB 색상 값을 인간이 인지하는 밝기로 변환하기 위한 가중치.
     * Rec.709(ITU-R BT.709) 표준.
     */
    float3 LUMINANCE = float3(0.299, 0.587, 0.114);
    
    float3 AccumulatedDiffuseColor = float3(0.0, 0.0, 0.0);
    float3 AccumulatedSpecularColor = float3(0.0, 0.0, 0.0);
    
    // 광원으로부터 계산된 스페큘러 값들 중 최대값
    float MaxObservedSpecularLuminance = 0.0f;

    
    // 조명 계산
    int BucketsPerTile = MAX_LIGHT_PER_TILE / 32;
    int StartIndex = TileIndex * BucketsPerTile;
    for (int Bucket = 0; Bucket < BucketsPerTile; ++Bucket)
    {
        int PointMask = PerTilePointLightIndexBuffer[StartIndex + Bucket];
        int SpotMask = PerTileSpotLightIndexBuffer[StartIndex + Bucket];
        for (int bit = 0; bit < 32; ++bit)
        {
            if (PointMask & (1u << bit))
            {
                // 전역 조명 인덱스는 bucket * 32 + bit 로 계산됨.
                // 전역 조명 인덱스가 총 조명 수보다 작은 경우에만 추가
                int GlobalPointLightIndex = Bucket * 32 + bit;
                if (GlobalPointLightIndex < MAX_LIGHT_PER_TILE) // TODO: MAX_LIGHT_PER_TILE 대신 실제 포인트 라이트 개수와 비교해야 함. (중요)
                {
                    FLightOutput Result = PointLight(
                        GlobalPointLightIndex, WorldPosition, WorldNormal, WorldViewPosition,
#ifdef LIGHTING_MODEL_PBR
                        BaseColor, Metallic, Roughness
#else
                        DiffuseColor, SpecularColor, Shininess
#endif
                    );
                    AccumulatedDiffuseColor += Result.DiffuseContribution;
                    AccumulatedSpecularColor += Result.SpecularContribution;
                    
                    MaxObservedSpecularLuminance = max(MaxObservedSpecularLuminance, dot(Result.SpecularContribution, LUMINANCE));
                }
            }
            if (SpotMask & (1u << bit))
            {
                int GlobalSpotLightIndex = Bucket * 32 + bit;
                if (GlobalSpotLightIndex < MAX_LIGHT_PER_TILE) // TODO: MAX_LIGHT_PER_TILE 대신 실제 스팟 라이트 개수와 비교해야 함. (중요)
                {
                    FLightOutput Result = SpotLight(
                        GlobalSpotLightIndex, WorldPosition, WorldNormal, WorldViewPosition,
#ifdef LIGHTING_MODEL_PBR
                        BaseColor, Metallic, Roughness
#else
                        DiffuseColor, SpecularColor, Shininess
#endif
                    );
                    AccumulatedDiffuseColor += Result.DiffuseContribution;
                    AccumulatedSpecularColor += Result.SpecularContribution;
                    
                    MaxObservedSpecularLuminance = max(MaxObservedSpecularLuminance, dot(Result.SpecularContribution, LUMINANCE));
                }
            }
        }
    }
    
    [unroll(MAX_DIRECTIONAL_LIGHT)]
    for (int k = 0; k < 1; k++) // TODO: 그림자는 0번 인덱스만 사용하더라도 실제 디렉셔널 라이트 개수만큼 루프해야함. (중요)
    {
         FLightOutput Result = DirectionalLight(
            k, WorldPosition, WorldNormal, WorldViewPosition,
#ifdef LIGHTING_MODEL_PBR
            BaseColor, Metallic, Roughness
#else
            DiffuseColor, SpecularColor, Shininess
#endif
        );
        AccumulatedDiffuseColor += Result.DiffuseContribution;
        AccumulatedSpecularColor += Result.SpecularContribution;

        MaxObservedSpecularLuminance = max(MaxObservedSpecularLuminance, dot(Result.SpecularContribution, LUMINANCE));
    }
    
    
    // 앰비언트
#ifdef LIGHTING_MODEL_PBR
    float3 IBL_DiffuseColor = float3(0.01, 0.01, 0.01); // TODO: 임시 값으로, 추후 IBL 적용

    if (AmbientLightsCount > 0)
    {
        IBL_DiffuseColor = Ambient[0].AmbientColor.rgb;
    }
    AccumulatedDiffuseColor += BaseColor * (1.0 - Metallic) * IBL_DiffuseColor;
#else
    float3 AmbientLightColor = float3(0.01, 0.01, 0.01);
    
    if (AmbientLightsCount > 0)
    {
        AmbientLightColor = Ambient[0].AmbientColor.rgb;
    }
    AccumulatedDiffuseColor += DiffuseColor * AmbientLightColor;
#endif

    float3 FinalRGB = AccumulatedDiffuseColor + AccumulatedSpecularColor;

    
    // 알파
    float3 V = normalize(WorldViewPosition - WorldPosition);
    float FinalAlpha = GetFinalAlpha(BaseAlpha, MaxObservedSpecularLuminance, V, WorldNormal,
#ifdef LIGHTING_MODEL_PBR
        BaseColor, Metallic
#else
        SpecularColor
#endif
    );

    
    return float4(FinalRGB, FinalAlpha);
}


float4 Lighting(float3 WorldPosition, float3 WorldNormal, float3 WorldViewPosition,
#ifdef LIGHTING_MODEL_PBR
    float3 BaseColor, float Metallic, float Roughness,
#else
    float3 DiffuseColor, float3 SpecularColor, float Shininess,
#endif
    float BaseAlpha
)
{
    /**
     * RGB 색상 값을 인간이 인지하는 밝기로 변환하기 위한 가중치.
     * Rec.709(ITU-R BT.709) 표준.
     */
    float3 LUMINANCE = float3(0.299, 0.587, 0.114);
    
    float3 AccumulatedDiffuseColor = float3(0.0, 0.0, 0.0);
    float3 AccumulatedSpecularColor = float3(0.0, 0.0, 0.0);
    
    // 광원으로부터 계산된 스페큘러 값들 중 최대값
    float MaxObservedSpecularLuminance = 0.0f;
    

    // 조명 계산
    // 다소 비효율적일 수도 있음.
    [unroll(MAX_POINT_LIGHT)]
    for (int i = 0; i < PointLightsCount; i++)
    {
        FLightOutput Result = PointLight(
            i, WorldPosition, WorldNormal, WorldViewPosition,
#ifdef LIGHTING_MODEL_PBR
            BaseColor, Metallic, Roughness
#else
            DiffuseColor, SpecularColor, Shininess
#endif
        );
        AccumulatedDiffuseColor += Result.DiffuseContribution;
        AccumulatedSpecularColor += Result.SpecularContribution;

        MaxObservedSpecularLuminance = max(MaxObservedSpecularLuminance, dot(Result.SpecularContribution, LUMINANCE));
    }

    [unroll(MAX_SPOT_LIGHT)]
    for (int j = 0; j < SpotLightsCount; j++)
    {
        FLightOutput Result = SpotLight(
            j, WorldPosition, WorldNormal, WorldViewPosition,
#ifdef LIGHTING_MODEL_PBR
            BaseColor, Metallic, Roughness
#else
            DiffuseColor, SpecularColor, Shininess
#endif
        );
        AccumulatedDiffuseColor += Result.DiffuseContribution;
        AccumulatedSpecularColor += Result.SpecularContribution;

        MaxObservedSpecularLuminance = max(MaxObservedSpecularLuminance, dot(Result.SpecularContribution, LUMINANCE));
    }
    
    [unroll(MAX_DIRECTIONAL_LIGHT)]
    for (int k = 0; k < 1; k++) 
    {
        FLightOutput Result = DirectionalLight(
            k, WorldPosition, WorldNormal, WorldViewPosition,
#ifdef LIGHTING_MODEL_PBR
            BaseColor, Metallic, Roughness
#else
            DiffuseColor, SpecularColor, Shininess
#endif
        );
        AccumulatedDiffuseColor += Result.DiffuseContribution;
        AccumulatedSpecularColor += Result.SpecularContribution;

        MaxObservedSpecularLuminance = max(MaxObservedSpecularLuminance, dot(Result.SpecularContribution, LUMINANCE));
    }


    // 앰비언트
#ifdef LIGHTING_MODEL_PBR
    float3 IBL_DiffuseColor = float3(0.01, 0.01, 0.01); // TODO: 임시 값으로, 추후 IBL 적용

    if (AmbientLightsCount > 0)
    {
        IBL_DiffuseColor = Ambient[0].AmbientColor.rgb;
    }
    AccumulatedDiffuseColor += BaseColor * (1.0 - Metallic) * IBL_DiffuseColor;
#else
    float3 AmbientLightColor = float3(0.01, 0.01, 0.01);
    
    if (AmbientLightsCount > 0)
    {
        AmbientLightColor = Ambient[0].AmbientColor.rgb;
    }
    AccumulatedDiffuseColor += DiffuseColor * AmbientLightColor;
#endif

    float3 FinalRGB = AccumulatedDiffuseColor + AccumulatedSpecularColor;

    
    // 알파
    float3 V = normalize(WorldViewPosition - WorldPosition);
    float FinalAlpha = GetFinalAlpha(BaseAlpha, MaxObservedSpecularLuminance, V, WorldNormal,
#ifdef LIGHTING_MODEL_PBR
        BaseColor, Metallic
#else
        SpecularColor
#endif
    );

    
    return float4(FinalRGB, FinalAlpha);
}
