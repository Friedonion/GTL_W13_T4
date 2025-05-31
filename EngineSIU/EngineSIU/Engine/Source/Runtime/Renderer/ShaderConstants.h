// Shader에 들어가는 (특히 static mesh) 상수 버퍼 구조체의 정의입니다.
// 추가하실 때 ShaderConstants.hlsli도 동일하게 맞춰주세요. <-- 삭제됨
// 슬롯 13번은 디버그 전용입니다.
// struct에 alignas가 붙어있으니 꼭 struct의 메모리 구조를 보면서 확인해주세요
// hlsl에서는 float3 float3을 연속으로 struct에 넣으면 자동으로 padding 4byte가 붙습니다.
// 이상한 값이 들어온다면 hlsl파일에 패딩을 넣어주세요.

#pragma once
#include "Math/Matrix.h"
#include "Math/Color.h"

// 아래의 두개 다 수정하기
#define MACRO_FCONSTANT_NUM_MAX_DIRLIGHT 10
#define MACRO_FCONSTANT_NUM_MAX_POINTLIGHT 10
#define MACRO_FCONSTANT_NUM_MAX_SPOTLIGHT 10

#define FCONSTANT_STRINGIFY(x) #x
#define FCONSTANT_TOSTRING(x) FCONSTANT_STRINGIFY(x)

// hlsl파일에 들어갈 macro define
constexpr D3D_SHADER_MACRO Defines[] =
{
    "FCONSTANT_NUM_DIRLIGHT", FCONSTANT_TOSTRING(MACRO_FCONSTANT_NUM_MAX_DIRLIGHT),
    "FCONSTANT_NUM_POINTLIGHT", FCONSTANT_TOSTRING(MACRO_FCONSTANT_NUM_MAX_POINTLIGHT),
    "FCONSTANT_NUM_SPOTLIGHT", FCONSTANT_TOSTRING(MACRO_FCONSTANT_NUM_MAX_SPOTLIGHT),
    nullptr, nullptr
};


struct FConstantBuffersStaticMesh
{
    ID3D11Buffer* Camera00;
    ID3D11Buffer* Light01;
    ID3D11Buffer* Actor03;
    ID3D11Buffer* Texture05;
    ID3D11Buffer* Mesh06;
};


struct FMaterialConstants2 {
    FVector DiffuseColor = { 0,0,0 };
    float TransparencyScalar = 0;
    FVector AmbientColor = { 0,0,0 };
    float DensityScalar = 0;
    FVector SpecularColor = { 0,0,0 };
    float SpecularScalar = 0;
    FVector EmmisiveColor = { 0,0,0 };
    float MaterialPad0;
};

////////////////////////////////////
// LIGHTS
struct alignas(16) FConstantBufferLightColor
{
    alignas(16) FVector Specular = { 0,0,0 };

    alignas(16) FVector Diffuse = { 0,0,0 };

    alignas(16) FVector Ambient = { 0,0,0 };
};

struct alignas(16) FConstantBufferLightDir
{
    FLinearColor Color;

    FVector Direction = { 0,0,0 };
    float Intensity = 0;
};

struct alignas(16) FConstantBufferLightPoint
{
    FLinearColor Color;
    FVector Position = { 0,0,0 };
    float pad0 = 0;

    float Intensity = 0;
    float Radius = 0;
    float RadiusFallOff = 0;
    float pad1 = 0;

};

struct alignas(16) FConstantBufferLightSpot
{
    FConstantBufferLightColor Color;
    alignas(16) float ConstantTerm = 0;
    float Linear = 0;
    float Quadratic = 0;
    float pad0 = 0;

    alignas(16) float CutOff = 0;
    float OuterCutOff = 0;
    float pad1 = 0;
    float pad2 = 0;

};


/// <summary>
/// Per-Mesh 상수버퍼 : b6
/// </summary>
struct alignas(16) FConstantBufferMesh
{
    FMatrix ModelMatrix;
    FMatrix ModelInvTransMatrix;

    FMaterialConstants2 Material;

    alignas(16) UINT IsSelectedMesh = 0;
    //FMatrix MVP;      // 모델
    //FMatrix ModelMatrixInverseTranspose; // normal 변환을 위한 행렬
    //FVector4 UUIDColor;
    //bool IsSelected;
    //FVector pad;
};

/// <summary>
/// Per-Texture 상수버퍼 : b5
/// </summary>
struct alignas(16) FConstantBufferTexture
{
    FVector2D UVOffset = { 0,0 };
};

/// <summary>
/// Per-Actor 상수버퍼 : b3
/// </summary>
struct alignas(16) FConstantBufferActor
{
    FVector4 UUID; // 임시
    UINT IsSelectedActor = 0;
};

/// <summary>
/// Lighting : b1
/// </summary>
struct alignas(16) FConstantBufferLights
{
    FConstantBufferLightDir DirLights[MACRO_FCONSTANT_NUM_MAX_DIRLIGHT];
    FConstantBufferLightPoint PointLights[MACRO_FCONSTANT_NUM_MAX_POINTLIGHT];
    FConstantBufferLightSpot SpotLights[MACRO_FCONSTANT_NUM_MAX_SPOTLIGHT];
    UINT IsLit = 1;
    UINT NumPointLights;
    UINT NumDirLights;
    float pad0 = 0.f;
};

/////////////////////////////////////////////////////////////////////////
// 디버그용
/////////////////////////////////////////////////////////////////////////

/// <summary>
/// Debug용 AABB 상수버퍼 : b11
/// </summary>
struct FConstantBufferDebugBox
{
    FMatrix WorldMatrix;
    
    FVector Extent;
    float Padding2 = 0.f;
};

/// <summary>
/// Debug용 Sphere 상수버퍼 : b11
/// </summary>
struct FConstantBufferDebugSphere
{
    FVector Position;
    float Radius = 0.f;
};

/// <summary>
/// Debug용 Cone 상수버퍼 : b11
/// </summary>
struct FConstantBufferDebugCone
{
    FVector ApexPosition;
    float Radius = 0.f;

    FVector Direction;
    float Angle = 0.f;
};

/// <summary>
/// Debug용 Grid 상수버퍼 : b11
/// </summary>
struct FConstantBufferDebugGrid
{
    FMatrix InverseViewProj;
};

/// <summary>
/// Debug용 Icon 상수버퍼 : b11
/// </summary>
struct FConstantBufferDebugIcon
{
    FVector Position;
    float Scale = 0.f;
};

/// <summary>
/// Debug용 Arrow 상수버퍼 : b11
/// </summary>
struct FConstantBufferDebugArrow
{
    FVector Position;
    float   ScaleXYZ = 0.f;
    
    FVector Direction;
    float   ScaleZ = 0.f;
};


struct FConstantBufferDebugCapsule
{
    FMatrix WorldMatrix;
    float Height = 0.f;
    float Radius = 0.f;
};

struct alignas(16) FConstantBufferCameraFade
{
    FLinearColor FadeColor = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);
    float  FadeAmount = 0.0f;
    FVector Padding;
};

struct alignas(16) FConstantBufferCameraVignette
{
    FLinearColor VignetteColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
    
    FVector2D VignetteCenter = FVector2D(0.5, 0.5);
    float VignetteRadius = 0.5;
    float VignetteSmoothness = 0.1f;
    
    float VignetteIntensity = 1.0f;
    FVector Padding;
};

struct alignas(16) FConstantBufferLetterBox
{
    FLinearColor LetterBoxColor = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);

    float ScreenAspectRatio;
    float LetterBoxAspectRatio = 16.f / 9.f;
    FVector2D Padding;
};

struct FDepthOfFieldConstant
{
    float F_Stop = 2.8f;
    float SensorWidth = 24.576f; // mm
    float FocalDistance = 0.f; // cm
    float FocalLength = 0.f; // mm

    float CoCScaleFactor = 2.f;
    float InFocusThreshold = 0.0f;
    float DOFPadding1 = 0.f;
    float DOFPadding2 = 0.f;
};
