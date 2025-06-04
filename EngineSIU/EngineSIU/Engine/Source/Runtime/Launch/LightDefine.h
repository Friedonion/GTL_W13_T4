#pragma once
#include "Math/Vector.h"
#include "Math/Vector4.h"
#include "Math/Matrix.h"
#include "CoreUObject/UObject/ObjectMacros.h"
#define MAX_AMBIENT_LIGHT 16
#define MAX_DIRECTIONAL_LIGHT 16
#define MAX_POINT_LIGHT 16
#define MAX_SPOT_LIGHT 16

struct FAmbientLightInfo
{
    FLinearColor AmbientColor;         // RGB + alpha
};


struct FDirectionalLightInfo
{
    DECLARE_STRUCT(FDirectionalLightInfo)

    FDirectionalLightInfo() = default;

    UPROPERTY(EditAnywhere, FLinearColor, LightColor, = FLinearColor())
    //FLinearColor LightColor;         // RGB + alpha


    UPROPERTY(EditAnywhere, FVector, Direction, = FVector(0,0,-1))
    UPROPERTY(EditAnywhere, float, Intensity, = 1.0f)
    //FVector Direction;   // 정규화된 광선 방향 (월드 공간 기준)
    //float   Intensity;   // 밝기



    // --- Shadow Info ---
    FMatrix LightViewProj; // 섀도우맵 생성 시 사용한 VP 행렬
    FMatrix LightInvProj;  // Light 광원 입장에서의 InvProj

    uint32 ShadowMapArrayIndex = 0 ;//캐스캐이드전 임시 배열
    uint32 CastShadows;

    UPROPERTY(EditAnywhere, float, ShadowBias, = 0.001f)
    //float ShadowBias;
    float Padding3; // 필요시

    // --- 직교 투영 파라미터 ---
    // 직교 투영 볼륨의 월드 단위 너비 (섀도우 영역)
    UPROPERTY(EditAnywhere, float, OrthoWidth, = 100.f)
    //float OrthoWidth = 100.0f;

    // 직교 투영 볼륨의 월드 단위 높이 (섀도우 영역)
    UPROPERTY(EditAnywhere, float, OrthoHeight, = 100.f)
    //float OrthoHeight = 100.0f;

    // 섀도우 계산을 위한 라이트 시점의 Near Plane (음수 가능)
    UPROPERTY(EditAnywhere, float, ShadowNearPlane, = 1.0f)
    //float ShadowNearPlane = 1.0F;

    // 섀도우 계산을 위한 라이트 시점의 Far Plane
    UPROPERTY(EditAnywhere, float, ShadowFarPlane, = 10000.f)
    //float ShadowFarPlane = 1000.0f;

};

struct FPointLightInfo
{
    FLinearColor LightColor;         // RGB + alpha

    FVector Position;    // 월드 공간 위치
    float   Radius;      // 감쇠가 0이 되는 거리

    int     Type;        // 라이트 타입 구분용 (예: 1 = Point)
    float   Intensity;   // 밝기
    float   Attenuation;
    float   Padding;  // 16바이트 정렬

    // --- Shadow Info ---
    FMatrix LightViewProjs[6]; // 섀도우맵 생성 시 사용한 VP 행렬
    
    uint32 CastShadows;
    float ShadowBias;
    uint32 ShadowMapArrayIndex = 0;
    float Padding2; // 필요시
};

struct FSpotLightInfo
{
    FLinearColor LightColor;         // RGB + alpha

    FVector Position;       // 월드 공간 위치
    float   Radius;         // 감쇠 거리

    FVector Direction;      // 빛이 향하는 방향 (normalize)
    float   Intensity;      // 밝기

    int     Type;           // 라이트 타입 구분용 (예: 2 = Spot)
    float   InnerRad; // cos(inner angle)
    float   OuterRad; // cos(outer angle)
    float   Attenuation;

    // --- Shadow Info ---
    FMatrix LightViewProj; // 섀도우맵 생성 시 사용한 VP 행렬
    
    uint32 CastShadows;
    float ShadowBias;
    uint32 ShadowMapArrayIndex;
    float Padding2; // 필요시
};

struct FLightInfoBuffer
{
    FAmbientLightInfo Ambient[MAX_AMBIENT_LIGHT];
    FDirectionalLightInfo Directional[MAX_DIRECTIONAL_LIGHT];
    FPointLightInfo PointLights[MAX_POINT_LIGHT];
    FSpotLightInfo SpotLights[MAX_SPOT_LIGHT];
    int DirectionalLightsCount;
    int PointLightsCount;
    int SpotLightsCount;
    int AmbientLightsCount;
};
