
#include "ShaderRegisters.hlsl"

Texture2D LayerInfoTexture : register(t90);
Texture2D BlurredNearTexture : register(t91);
Texture2D BlurredFarTexture : register(t92);
Texture2D FilteredCoCTexture : register(t93);
Texture2D BlurredCoCTexture : register(t94);
Texture2D InputDepth : register(t99);
Texture2D SceneTexture : register(t100);

SamplerState LinearSampler : register(s1); // Linear Clamp
SamplerState PointSampler : register(s3); // Point Clamp

static const float MAX_KERNEL_PIXEL_RADIUS = 5.0f;

// 포아송 디스크 샘플링 패턴 (19개 샘플)
static const int NUM_SAMPLES = 19;
static const float2 PoissonSamples[NUM_SAMPLES] = {
    float2(0.0f, 0.0f),        // Center
    float2(0.527837f, -0.085868f), float2(-0.040088f, 0.536087f), float2(-0.670445f, -0.179949f),
    float2(-0.419418f, -0.616039f), float2(0.440453f, -0.639399f), float2(-0.757088f, 0.349334f),
    float2(0.574619f, 0.685879f), float2(0.976331f, 0.15346f), float2(-0.624817f, 0.765323f),
    float2(0.122747f, 0.970479f), float2(0.840895f, -0.52498f), float2(-0.043655f, -0.967251f),
    float2(-0.848312f, -0.519516f), float2(-0.998088f, 0.054414f), float2(0.285328f, 0.418364f),
    float2(-0.273026f, -0.340141f), float2(0.725791f, 0.326734f), float2(-0.311553f, 0.148081f)
};

cbuffer TextureSize : register(b0)
{
    float2 TexturePixelSize;
    float BokehIntensityScale;
    float TextureSizePadding;
}

cbuffer DepthOfFieldConstant : register(b1)
{
    float F_Stop;
    float SensorWidth_mm; // mm
    float FocalDistance_World; // cm
    float FocalLength_mm; // mm

    float CoCScaleFactor;
    float InFocusThreshold; // [0, 1]
    float DOFPadding1;
    float DOFPadding2;
};

struct PS_Input
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD;
};

float LinearizeDepth(float NonLinearDepth, float Near, float Far)
{
    return (Near * Far) / (max(0.0001f, Far - NonLinearDepth * (Far - Near)));
}

float4 CalculateNearBlur(float2 TexUV, float2 CurrentTextureSize)
{
    float4 CenterPixelData = SceneTexture.Sample(LinearSampler, TexUV);
    float3 CenterColor = CenterPixelData.rgb;
    float CenterCoC = CenterPixelData.a;

    if (CenterCoC < 0.001f)
    {
        return CenterPixelData;
    }

    float3 AccumulatedColor = float3(0.0f, 0.0f, 0.0f);
    float TotalWeight = 0.0f;
    float MaxCoC = CenterCoC; // 최대 CoC 추적

    // 확장된 블러 반경 - 주변 픽셀들의 CoC도 고려
    float ActualPixelRadius = CenterCoC * MAX_KERNEL_PIXEL_RADIUS * BokehIntensityScale;

    for (int i = 0; i < NUM_SAMPLES; ++i)
    {
        // 1차: 기본 반경으로 샘플링
        float2 PixelOffset = PoissonSamples[i] * ActualPixelRadius;
        float2 UVOffset = PixelOffset / CurrentTextureSize;
        float2 SampleUV = TexUV + UVOffset;

        float4 SampleData = SceneTexture.Sample(LinearSampler, SampleUV);
        float3 SampleColor = SampleData.rgb;
        float SampleCoC = SampleData.a;
        
        float Weight = 1.0f;

        // 샘플 픽셀의 CoC가 중심보다 클 경우, 그 영향 반경 계산
        float SampleInfluenceRadius = SampleCoC * MAX_KERNEL_PIXEL_RADIUS * BokehIntensityScale;
        // 현재 샘플 위치에서 중심까지의 거리
        float DistanceToCenter = length(PixelOffset);

        // Near 레이어 전용 가중치 계산
        // 1. 샘플 픽셀이 중심에 영향을 줄 수 있는지 확인
        if (DistanceToCenter <= SampleInfluenceRadius && SampleInfluenceRadius > 0.001f)
        {
            // 샘플 픽셀의 영향권 내에 있으면 높은 가중치
            Weight *= saturate(1.0f - (DistanceToCenter / SampleInfluenceRadius));
        }
        else
        {
            // 기본 가우시안 가중치
            float Sigma = 0.5f; // 2.0f * 0.5f * 0.5f = 0.5f
            float NormalizedKernelDistance = length(PoissonSamples[i]);
            Weight *= exp(-NormalizedKernelDistance * NormalizedKernelDistance / Sigma);
        }

        // 하이라이트 부스트
        float HighlightBoost = 1.0f + saturate(dot(SampleColor, SampleColor) - 0.8f) * 5.0f;
        Weight *= HighlightBoost;

        // CoC 기반 가중치 (Near 레이어 전용)
        // 샘플의 CoC가 클수록 더 많이 기여하도록
        Weight *= saturate(SampleCoC * 2.0f + 0.1f);

        if (Weight > 0.0001f)
        {
            AccumulatedColor += SampleColor * Weight;
            TotalWeight += Weight;
            MaxCoC = max(MaxCoC, SampleCoC);
        }
    }

    if (TotalWeight > 0.001f)
    {
        AccumulatedColor /= TotalWeight;
    }
    else
    {
        return CenterPixelData;
    }

    return float4(AccumulatedColor, MaxCoC);
}

float4 CalculateFarBlur(float2 TexUV, float2 CurrentTextureSize)
{
    float4 CenterPixelData = SceneTexture.Sample(LinearSampler, TexUV);
    float3 CenterColor = CenterPixelData.rgb;
    float CenterCoC = CenterPixelData.a;

    if (CenterCoC < 0.001f)
    {
        return CenterPixelData;
    }

    float3 AccumulatedColor = float3(0.0f, 0.0f, 0.0f);
    float TotalWeight = 0.0f;

    float ActualPixelRadius = CenterCoC * MAX_KERNEL_PIXEL_RADIUS * BokehIntensityScale;

    for (int i = 0; i < NUM_SAMPLES; ++i)
    {
        float2 PixelOffset = PoissonSamples[i] * ActualPixelRadius;
        float2 UVOffset = PixelOffset / CurrentTextureSize;
        float2 SampleUV = TexUV + UVOffset;

        float4 SampleData = SceneTexture.Sample(LinearSampler, SampleUV);
        float3 SampleColor = SampleData.rgb;
        float SampleCoC = SampleData.a;

        float Weight = 1.0f;

        // 거리 기반 가우시안 가중치
        float Sigma = 0.5f; // 2.0f * 0.5f * 0.5f = 0.5f
        float NormalizedKernelDistanceSq = dot(PoissonSamples[i], PoissonSamples[i]);
        float GaussianWeight = exp(-NormalizedKernelDistanceSq / Sigma);
        Weight *= GaussianWeight;

        // 하이라이트 부스트
        float HighlightBoost = 1.0f + saturate(dot(SampleColor, SampleColor) - 0.8f) * 5.0f;
        Weight *= HighlightBoost;

        // Far 레이어의 경우 기존 로직 유지 (배경 누출 방지)
        if (CenterCoC > 0.01f)
        {
            float Ratio = (SampleCoC + 0.01f) / (CenterCoC + 0.01f);
            Weight *= smoothstep(0.1f, 0.5f, Ratio);
        }
        else
        {
            Weight *= smoothstep(0.2f, 0.0f, SampleCoC);
        }

        if (Weight > 0.0001f)
        {
            AccumulatedColor += SampleColor * Weight;
            TotalWeight += Weight;
        }
    }

    if (TotalWeight > 0.001f)
    {
        AccumulatedColor /= TotalWeight;
    }
    else
    {
        return CenterPixelData;
    }

    return float4(AccumulatedColor, CenterCoC);
}

float4 CalculateLayerCoCAndMasks(float2 UV)
{
    float4 LayerInfo = float4(0.0f, 0.0f, 0.0f, 0.0f);

    // 유효성 검사
    if (FocalLength_mm < 0.01f || FocalDistance_World < 0.01f || F_Stop < 0.01f ||
        SensorWidth_mm < 0.01f || NearClip <= 0.0f || FarClip <= NearClip || CoCScaleFactor < 0.0f)
    {
        LayerInfo.z = 1.0f; // 문제가 있을 경우, 기본적으로 모든 것을 초점 맞음으로 처리
        return LayerInfo;
    }
    
    const float NonLinearDepth = InputDepth.Sample(PointSampler, UV).r;
    // LinearizeDepth에 전달하는 NearClip, FarClip은 뎁스 버퍼가 생성될 때 사용된 카메라의 값과 일치해야 합니다.
    // 또한, FocalDistance_World와 단위가 같아야 합니다.
    const float SceneDistance_World = LinearizeDepth(NonLinearDepth, NearClip, FarClip);

    // 모든 단위를 mm로 통일
    const float FocalDistance_mm = FocalDistance_World * 10.0f; // cm -> mm (월드 단위가 cm라고 가정)
    const float SceneDistance_mm = SceneDistance_World * 10.0f; // cm -> mm (월드 단위가 cm라고 가정)
    
    float ScaledSignedCoc = 0.0f; // 최종적으로 스케일링된 부호 있는 CoC

    // SceneDistance_mm가 유효한 값일 때만 계산 (예: 매우 큰 값 방지)
    // FarClip을 초과하는 거리는 FarClip에서의 CoC로 clamp하거나, 다른 처리가 필요할 수 있음.
    // 현재는 LinearizeDepth 결과 그대로 사용.
    if (SceneDistance_mm > 0.001f) // 유효한 거리인지 확인
    {
        // (SceneDistance_mm - FocalLength_mm)가 0 또는 음수가 되는 것을 방지 (특히 근거리 물체)
        // 실제 물리 기반 공식 대신 근사 공식을 사용하는 경우가 많음.
        // 기존 코드의 근사 분모: FocalDistance_mm * max(0.001f, SceneDistance_mm)
        const float Denominator = FocalDistance_mm * max(0.001f, SceneDistance_mm);
        const float SignedDistanceDiff_mm = SceneDistance_mm - FocalDistance_mm;

        if (abs(Denominator) > 0.00001f) // 분모가 0에 매우 가깝지 않은지 확인
        {
            // CoC 직경 (mm 단위, 센서 평면 기준), 부호 유지
            // CoC_sensor_signed = (FocalLength^2 / F_Stop) * (SceneDistance - FocalDistance) / Denominator
            float CoC_sensor_signed = (FocalLength_mm * FocalLength_mm / F_Stop) * SignedDistanceDiff_mm / Denominator;

            // 센서 크기로 정규화 (부호 유지)
            float NormalizedSignedCoc = CoC_sensor_signed / SensorWidth_mm;
            
            // 제공된 스케일 팩터 적용
            ScaledSignedCoc = NormalizedSignedCoc * CoCScaleFactor;
        }
        else if (abs(SignedDistanceDiff_mm) > 0.01f) // 분모 문제 발생 시, 거리 차이가 있다면 최대 CoC 부여
        {
            // CoCScaleFactor가 양수라고 가정
            ScaledSignedCoc = (SignedDistanceDiff_mm > 0 ? 1.0f : -1.0f) * CoCScaleFactor; // 부호 있는 최대 스케일 CoC
        }
        // 그 외의 경우는 ScaledSignedCoc = 0.0f 유지
    }
    // SceneDistance_mm <= 0.001f (예: 너무 가깝거나 잘못된 뎁스)인 경우 ScaledSignedCoc = 0.0f 유지

    LayerInfo.w = saturate(ScaledSignedCoc * 0.5f + 0.5f); // Raw Scaled Signed CoC 저장

    float CocMagnitude = abs(ScaledSignedCoc);
    float SaturatedCocMagnitude = saturate(CocMagnitude); // 블러 강도로 사용될 값 (0~1)

    if (SaturatedCocMagnitude < InFocusThreshold)
    {
        LayerInfo.z = 1.0f; // In-focus
        // LayerInfo.x (FarCoC) and LayerInfo.y (NearCoC) remain 0.0
    }
    else
    {
        LayerInfo.z = 0.0f; // Out-of-focus
        if (ScaledSignedCoc > 0.0f) // Far field (SceneDistance > FocalDistance)
        {
            LayerInfo.x = SaturatedCocMagnitude; // Far CoC
        }
        else // Near field (ScaledSignedCoc < 0.0f, SceneDistance < FocalDistance)
        {
            LayerInfo.y = SaturatedCocMagnitude; // Near CoC
        }
    }
    
    return LayerInfo;
}

float4 PS_GenerateLayer(PS_Input Input) : SV_TARGET
{
    /**
     * R: Far Field Coc
     * G: Near Field Coc
     * B: In-focus Mask
     * A: Raw Signed CoC
     */
    return CalculateLayerCoCAndMasks(Input.UV);
}

float4 PS_FilterNearCoC_Max(PS_Input Input) : SV_TARGET
{
    float2 UV = Input.UV;
    float2 TexelSize = TexturePixelSize; 

    float MaxNearCoC = 0.0f;

    // 3x3 Max Filter 예시 (커널 크기는 필요에 따라 조절)
    // 좀 더 넓은 영역을 원하면 5x5 등으로 확장 가능
    const int KernelRadius = 2; // 3x3 ( -1, 0, 1 )
    for (int y = -KernelRadius; y <= KernelRadius; ++y)
    {
        for (int x = -KernelRadius; x <= KernelRadius; ++x)
        {
            float2 Offset = float2(x, y) * TexelSize;
            // LayerInfoTexture 샘플링 시 PointSampler 사용 권장
            float CurrentNearCoC = LayerInfoTexture.Sample(PointSampler, UV + Offset).g; 
            MaxNearCoC = max(MaxNearCoC, CurrentNearCoC);
        }
    }
    
    // 필터링된 MaxNearCoC 값을 예를 들어 R 채널에 저장합니다.
    // 다른 채널 값은 필요에 따라 LayerInfoTexture에서 가져오거나 0으로 설정할 수 있습니다.
    // 여기서는 다른 채널은 사용하지 않는다고 가정하고 MaxNearCoC만 R 채널에 씁니다.
    return float4(MaxNearCoC, MaxNearCoC, MaxNearCoC, MaxNearCoC);
}

float4 PS_BlurCoCMap(PS_Input Input) : SV_TARGET
{
    float2 UV = Input.UV;
    float2 TexelSize = TexturePixelSize; // FilteredCoCTexture의 텍셀 크기

    float AccumulatedCoC = 0.0f;
    float TotalWeight = 0.0f;

    // 5x5 가우시안 블러 커널
    // 가중치 합계 = 256
    // (1  4  6  4  1)
    // (4 16 24 16  4)
    // (6 24 36 24  6) * (1/256)
    // (4 16 24 16  4)
    // (1  4  6  4  1)
    const float Weights[5][5] = {
        { 1.0f/256.0f,  4.0f/256.0f,  6.0f/256.0f,  4.0f/256.0f,  1.0f/256.0f },
        { 4.0f/256.0f, 16.0f/256.0f, 24.0f/256.0f, 16.0f/256.0f,  4.0f/256.0f },
        { 6.0f/256.0f, 24.0f/256.0f, 36.0f/256.0f, 24.0f/256.0f,  6.0f/256.0f },
        { 4.0f/256.0f, 16.0f/256.0f, 24.0f/256.0f, 16.0f/256.0f,  4.0f/256.0f },
        { 1.0f/256.0f,  4.0f/256.0f,  6.0f/256.0f,  4.0f/256.0f,  1.0f/256.0f }
    };
    const int KernelRadius = 2; // 5x5 커널의 경우 반지름은 2 (인덱스: -2, -1, 0, 1, 2)

    for (int y = -KernelRadius; y <= KernelRadius; ++y)
    {
        for (int x = -KernelRadius; x <= KernelRadius; ++x)
        {
            float2 Offset = float2(x, y) * TexelSize;
            // FilteredCoCTexture는 보통 R 채널에 CoC 값을 저장합니다.
            // (PS_FilterNearCoC_Max에서 그렇게 저장했다면)
            // LinearSampler를 사용하면 픽셀 간 부드러운 보간이 약간 도움될 수 있습니다.
            float SampledCoC = FilteredCoCTexture.Sample(LinearSampler, UV + Offset).r; 
            float CurrentWeight = Weights[y + KernelRadius][x + KernelRadius];
            
            AccumulatedCoC += SampledCoC * CurrentWeight;
            TotalWeight += CurrentWeight; // 정규화된 가중치를 사용하면 이 줄은 필요 없을 수도 있습니다.
        }
    }

    // TotalWeight가 거의 0이 아닐 경우에만 나누기 (정규화된 가중치 합이 1이면 불필요)
    // if (TotalWeight > 0.0001f)
    // {
    //     AccumulatedCoC /= TotalWeight;
    // }

    // 블러 처리된 CoC 값을 R 채널에 다시 저장 (또는 사용하는 채널)
    // 다른 채널 값은 원본 LayerInfoTexture에서 가져오거나, 사용하지 않으면 0으로 설정
    float4 OriginalLayerInfo = LayerInfoTexture.Sample(PointSampler, UV); // 필요시 원본 다른 정보 가져오기
    
    return float4(AccumulatedCoC, AccumulatedCoC, AccumulatedCoC, AccumulatedCoC);
}

float4 PS_BlurNearLayer(PS_Input Input) : SV_TARGET
{
    float2 DownSampleTextureSize = 1.0 / TexturePixelSize; // 실제 텍스처 크기
    return CalculateNearBlur(Input.UV, DownSampleTextureSize);
}

float4 PS_BlurFarLayer(PS_Input Input) : SV_TARGET
{
    float2 DownSampleTextureSize = 1.0 / TexturePixelSize; // 실제 텍스처 크기
    return CalculateFarBlur(Input.UV, DownSampleTextureSize);
}

float4 PS_ExtractAndDownsampleLayer(PS_Input Input) : SV_TARGET
{
    float2 UV = Input.UV;
    float2 TexelSize = TexturePixelSize;

    // 2x2 샘플링 오프셋 (풀 해상도 텍셀 기준)
    float2 Offsets[4] =
    {
        float2(-0.5f * TexelSize.x, -0.5f * TexelSize.y),
        float2( 0.5f * TexelSize.x, -0.5f * TexelSize.y),
        float2(-0.5f * TexelSize.x,  0.5f * TexelSize.y),
        float2( 0.5f * TexelSize.x,  0.5f * TexelSize.y)
    };

    float3 AccumulatedColor = float3(0.0f, 0.0f, 0.0f);
    float TotalWeight = 0.0f;
    float RepresentativeCoC = 0.0f; // 합성 시 사용할 CoC 대표값 (예: 최대값)

    for (int i = 0; i < 4; ++i)
    {
        float2 SampleUV = UV + Offsets[i];

        float CurrentCoC = 
#ifdef NEAR
            BlurredCoCTexture.Sample(LinearSampler, SampleUV).r;
#else
            LayerInfoTexture.Sample(LinearSampler, SampleUV).r;
#endif

        float4 CurrentSceneColor = SceneTexture.Sample(LinearSampler, SampleUV);
        AccumulatedColor += CurrentSceneColor.rgb; // CoC를 가중치로 사용
        TotalWeight += 1.0;
        RepresentativeCoC = max(RepresentativeCoC, CurrentCoC);
    }

    if (TotalWeight > 0.001f)
    {
        return float4(AccumulatedColor / TotalWeight, RepresentativeCoC); // 또는 totalNearWeight / 4.0f 등
    }
    
    return float4(0.0f, 0.0f, 0.0f, 0.0f);
}

float4 DebugCoC(float2 UV)
{
    float4 layerInfo = LayerInfoTexture.Sample(LinearSampler, UV); // 또는 LinearClampSampler
    
    float farCoC = layerInfo.r;        // 0.0 ~ 1.0 (원경 흐림 강도)
    float nearCoC = layerInfo.g;       // 0.0 ~ 1.0 (근경 흐림 강도)
    float inFocusMask = layerInfo.b;   // 0.0 또는 1.0 (초점 마스크)
    float mappedSignedCoC = layerInfo.a; // 0.0 ~ 1.0 (부호있는 CoC를 0-1로 매핑한 값, 0.5가 초점 근처)

    // --- 디버깅 출력 선택 ---

    // 예시 1: Far CoC 값을 회색조로 표시
    // return float4(farCoC, farCoC, farCoC, 1.0f);

    // 예시 2: Near CoC 값을 회색조로 표시
    // return float4(nearCoC, nearCoC, nearCoC, 1.0f);

    // 예시 3: In-Focus Mask를 표시 (초점 영역은 흰색, 나머지는 검은색)
    // return float4(inFocusMask, inFocusMask, inFocusMask, 1.0f);
    
    // 예시 4: Mapped Signed CoC 값을 회색조로 표시
    // (0.5 근처는 중간 회색, 0은 검은색, 1은 흰색)
    //return float4(mappedSignedCoC, mappedSignedCoC, mappedSignedCoC, 1.0f);

    // 예시 5: 니어/파/인포커스를 다른 색으로 표시
    float3 debugColor = float3(0.0f, 0.0f, 0.0f);
    if (inFocusMask > 0.5f) // 초점 영역
    {
        //debugColor = float3(0.0f, 1.0f, 0.0f); // 녹색
    }
    else if (nearCoC > 0.01f) // 근경 아웃포커스
    {
        // nearCoC 값에 따라 밝기 조절 가능
        debugColor = float3(0.f, nearCoC, 0.0f); // 빨간색 (밝을수록 강한 근경 CoC)
    }
    else if (farCoC > 0.01f) // 원경 아웃포커스
    {
        // farCoC 값에 따라 밝기 조절 가능
        debugColor = float3(0.0f, 0.0f, farCoC); // 파란색 (밝을수록 강한 원경 CoC)
    }
    return float4(debugColor, 1.0f);

    // 예시 6: 원본 Signed CoC 값을 (어느 정도) 복원하여 표시 (MappedSignedCoC 사용)
    // DepthOfFieldConstant 버퍼에서 MaxAbsCocForAlphaEncoding 값을 가져와야 함
    // float maxAbsCoc = MaxAbsCocForAlphaEncoding; // 상수 버퍼에서 가져온 값이라고 가정
    // float originalScaledSignedCoC = (mappedSignedCoC * 2.0f - 1.0f) * maxAbsCoc;
    // // 시각화를 위해 0 중심으로 다시 매핑 (예: -maxAbsCoc -> 0, 0 -> 0.5, +maxAbsCoc -> 1)
    // float visualizedSignedCoC = saturate((originalScaledSignedCoC / maxAbsCoc) * 0.5f + 0.5f);
    // return float4(visualizedSignedCoC, visualizedSignedCoC, visualizedSignedCoC, 1.0f);

    // 예시 7: FarCoC는 R, NearCoC는 G, InFocusMask는 B에 표시
    // return float4(farCoC, nearCoC, inFocusMask, 1.0f);
}

float4 PS_Composite(PS_Input Input) : SV_TARGET
{
    // 1. 원본 해상도 레이어 정보 가져오기
    // LayerInfoTexture는 픽셀 단위의 정확한 마스크 정보이므로 PointSampler를 사용할 수 있으나,
    // 약간의 부드러운 전환을 위해 LinearSampler를 사용해도 무방합니다.
    // 여기서는 생성 시 의도에 따라 PointSampler 또는 LinearSampler를 선택합니다.
    // 일반적으로 CoC 값은 부드럽게 변하므로 LinearSampler가 더 적합할 수 있습니다.
    float4 LayerInfo = LayerInfoTexture.Sample(LinearSampler, Input.UV);
    float FarCoC = LayerInfo.r;
    float NearCoC = LayerInfo.g;
    float InFocusMask = LayerInfo.b; // 0.0 (아웃포커스) 또는 1.0 (인포커스)

    // 2. 원본 씬 컬러 가져오기
    float4 OriginalSceneColor = SceneTexture.Sample(LinearSampler, Input.UV);

    // 3. 블러된 파 레이어 컬러 가져오기 (업샘플링 발생)
    // BlurredFarLayerTexture의 알파 채널은 FarCoC를 담고 있을 수 있음 (블러 쉐이더 설계에 따라).
    // 여기서는 해당 CoC 값을 블러 강도에 이미 반영했다고 가정하고, RGB만 사용.
    // 또는, 해당 알파값을 추가적인 가중치로 사용할 수도 있습니다.
    float4 BlurredFarColor = BlurredFarTexture.Sample(LinearSampler, Input.UV);
    float BlurredFarCoC = BlurredFarColor.a;

    // 4. 블러된 니어 레이어 컬러 가져오기 (업샘플링 발생)
    float4 BlurredNearColor = BlurredNearTexture.Sample(LinearSampler, Input.UV);
    float BlurredNearCoC = BlurredNearColor.a;

    // 5. 합성 로직
    float3 FinalColor = OriginalSceneColor.rgb;

    // Far 레이어 합성:
    float FarBlendFactor = saturate((1.0f - InFocusMask) * BlurredFarCoC);
    FinalColor = lerp(FinalColor, BlurredFarColor.rgb, FarBlendFactor);

    // Near 레이어 합성:
    float NearBlendFactor = saturate((1.0f - InFocusMask) * BlurredNearCoC);
    FinalColor = lerp(FinalColor, BlurredNearColor.rgb, NearBlendFactor);

    // (선택적) 최종적으로 초점 영역은 원본을 확실히 보장 (만약 위 lerp들이 완벽히 0이나 1이 안될 경우 대비)
    // finalColor = lerp(finalColor, originalSceneColor.rgb, inFocusMask); // inFocusMask가 1이면 originalSceneColor 보장
    
    //return DebugCoC(Input.UV);

    // 최종 알파는 원본 씬의 알파를 사용
    return float4(FinalColor, OriginalSceneColor.a);
}
