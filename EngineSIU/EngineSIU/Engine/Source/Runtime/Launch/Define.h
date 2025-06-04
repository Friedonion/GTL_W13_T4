#pragma once
#include <cmath>
#include <algorithm>
#include "Core/Container/String.h"
#include "Core/Container/Array.h"
#include "UObject/NameTypes.h"

// 수학 관련
#include "Math/Vector.h"
#include "Math/Vector4.h"
#include "Math/Matrix.h"

#define _TCHAR_DEFINED
#include <d3d11.h>

#include "UserInterface/Console.h"
#include <Math/Color.h>

#define GOURAUD "LIGHTING_MODEL_GOURAUD"
#define LAMBERT "LIGHTING_MODEL_LAMBERT"
#define PHONG "LIGHTING_MODEL_BLINN_PHONG"
#define PBR "LIGHTING_MODEL_PBR"

// Material Subset
struct FMaterialSubset
{
    uint32 IndexStart; // Index Buffer Start pos
    uint32 IndexCount; // Index Count
    uint32 MaterialIndex; // Material Index
    FString MaterialName; // Material Name

    friend FArchive& operator<<(FArchive& Ar, FMaterialSubset& Data)
    {
        return Ar << Data.IndexStart
                  << Data.IndexCount
                  << Data.MaterialIndex
                  << Data.MaterialName;
    }
};

struct FStaticMaterial
{
    class UMaterial* Material;
    FName MaterialSlotName;
};

// OBJ File Raw Data
struct FObjInfo
{
    FWString ObjectName; // OBJ File Name. Path + FileName.obj 
    FWString FilePath; // OBJ File Paths
    FString DisplayName; // Display Name
    FString MatName; // OBJ MTL File Name

    // Group
    uint32 NumOfGroup = 0; // token 'g' or 'o'
    TArray<FString> GroupName;

    // Vertex, UV, Normal List
    TArray<FVector> Vertices;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;

    // Faces
    TArray<int32> Faces;

    // Index
    TArray<uint32> VertexIndices;
    TArray<uint32> NormalIndices;
    TArray<uint32> UVIndices;

    // Material
    TArray<FMaterialSubset> MaterialSubsets;
};

enum class EMaterialTextureFlags : uint16
{
    MTF_Diffuse      = 1 << 0,
    MTF_Specular     = 1 << 1,
    MTF_Normal       = 1 << 2,
    MTF_Emissive     = 1 << 3,
    MTF_Alpha        = 1 << 4,
    MTF_Ambient      = 1 << 5,
    MTF_Shininess    = 1 << 6,
    MTF_Metallic     = 1 << 7,
    MTF_Roughness    = 1 << 8,
    MTF_MAX,
};

enum class EMaterialTextureSlots : uint8
{
    MTS_Diffuse      = 0,
    MTS_Specular     = 1,
    MTS_Normal       = 2,
    MTS_Emissive     = 3,
    MTS_Alpha        = 4,
    MTS_Ambient      = 5,
    MTS_Shininess    = 6,
    MTS_Metallic     = 7,
    MTS_Roughness    = 8,
    MTS_MAX,
};

struct FTextureInfo
{
    FString TextureName;
    FWString TexturePath;
    bool bIsSRGB;

    friend FArchive& operator<<(FArchive& Ar, FTextureInfo& Info)
    {
        FString TexturePathStr = Info.TexturePath;

        Ar << Info.TextureName
           << TexturePathStr
           << Info.bIsSRGB;

        Info.TexturePath = TexturePathStr.ToWideString();
        
        return Ar;
    }
};

struct FMaterialInfo
{
    FString MaterialName;  // newmtl: Material Name.

    uint32 TextureFlag = 0;

    bool bTransparent = false; // Has alpha channel?

    FVector DiffuseColor = FVector(0.7f, 0.7f, 0.7f);      // Kd: Diffuse Color
    FVector SpecularColor = FVector(0.5f, 0.5f, 0.5f);     // Ks: Specular Color
    FVector AmbientColor = FVector(0.01f, 0.01f, 0.01f);   // Ka: Ambient Color
    FVector EmissiveColor = FVector::ZeroVector;                   // Ke: Emissive Color

    float Shininess = 250.f;                                // Ns: Specular Power
    float IOR = 1.5f;                                              // Ni: Index of Refraction
    float Transparency = 0.f;                                      // d or Tr: Transparency of surface
    float BumpMultiplier = 1.f;                                    // -bm: Bump Multiplier
    uint32 IlluminanceModel = 0;                                       // illum: illumination Model between 0 and 10.

    float Metallic = 0.0f;                                         // Pm: Metallic
    float Roughness = 0.5f;                                        // Pr: Roughness
    
    /* Texture */
    TArray<FTextureInfo> TextureInfos;

    void Serialize(FArchive& Ar)
    {
        Ar << MaterialName
           << TextureFlag
           << bTransparent
           << DiffuseColor
           << SpecularColor
           << AmbientColor
           << EmissiveColor
           << Shininess
           << IOR
           << Transparency
           << BumpMultiplier
           << IlluminanceModel
           << Metallic
           << Roughness
           << TextureInfos;
    }

    friend FArchive& operator<<(FArchive& Ar, FMaterialInfo& Info)
    {
        Info.Serialize(Ar);
        return Ar;
    }
};

struct FVertexTexture
{
    float x, y, z;    // Position
    float u, v; // Texture
};

struct FGridParameters
{
    float GridSpacing;
    int   NumGridLines;
    FVector2D Padding1;
    
    FVector GridOrigin;
    float pad;
};

struct FSimpleVertex
{
    float dummy; // 내용은 사용되지 않음
    float padding[11];
};

struct FOBB
{
    FVector4 corners[8];
};

struct FRect
{
    FRect() : TopLeftX(0), TopLeftY(0), Width(0), Height(0) {}
    FRect(float x, float y, float w, float h) : TopLeftX(x), TopLeftY(y), Width(w), Height(h) {}
    float TopLeftX, TopLeftY, Width, Height;
};

struct FPoint
{
    FPoint() : x(0), y(0) {}
    FPoint(float _x, float _y) : x(_x), y(_y) {}
    FPoint(long _x, long _y) : x(static_cast<float>(_x)), y(static_cast<float>(_y)) {}
    FPoint(int _x, int _y) : x(static_cast<float>(_x)), y(static_cast<float>(_y)) {}

    float x, y;
};

struct FBoundingBox
{
    FBoundingBox() = default;
    FBoundingBox(FVector InMin, FVector InMax) : MinLocation(InMin), MaxLocation(InMax) {}
    
    FVector MinLocation; // Minimum extents
    float pad = 0;
    
    FVector MaxLocation; // Maximum extents
    float pad1 = 0;

    bool IsValidBox() const
    {
        return MinLocation.X <= MaxLocation.X && MinLocation.Y <= MaxLocation.Y && MinLocation.Z <= MaxLocation.Z;
    }

    static bool CheckOverlap(const FBoundingBox& A, const FBoundingBox& B)
    {
        if (A.MaxLocation.X < B.MinLocation.X || A.MinLocation.X > B.MaxLocation.X)
        {
            return false;
        }
        if (A.MaxLocation.Y < B.MinLocation.Y || A.MinLocation.Y > B.MaxLocation.Y)
        {
            return false;
        }
        if (A.MaxLocation.Z < B.MinLocation.Z || A.MinLocation.Z > B.MaxLocation.Z)
        {
            return false;
        }
        return true;
    }
    
    bool Intersect(const FVector& RayOrigin, const FVector& RayDir, float& OutDistance) const
    {
        float TMin = -FLT_MAX;
        float TMax = FLT_MAX;
        constexpr float epsilon = 1e-6f;

        // X축 처리
        if (FMath::Abs(RayDir.X) < epsilon)
        {
            // 레이가 X축 방향으로 거의 평행한 경우,
            // 원점의 x가 박스 [min.X, max.X] 범위 밖이면 교차 없음
            if (RayOrigin.X < MinLocation.X || RayOrigin.X > MaxLocation.X)
            {
                return false;
            }
        }
        else
        {
            float T1 = (MinLocation.X - RayOrigin.X) / RayDir.X;
            float T2 = (MaxLocation.X - RayOrigin.X) / RayDir.X;
            if (T1 > T2)
            {
                std::swap(T1, T2);
            }

            // tmin은 "현재까지의 교차 구간 중 가장 큰 min"
            TMin = (T1 > TMin) ? T1 : TMin;
            // tmax는 "현재까지의 교차 구간 중 가장 작은 max"
            TMax = (T2 < TMax) ? T2 : TMax;
            if (TMin > TMax)
            {
                return false;
            }
        }

        // Y축 처리
        if (FMath::Abs(RayDir.Y) < epsilon)
        {
            if (RayOrigin.Y < MinLocation.Y || RayOrigin.Y > MaxLocation.Y)
            {
                return false;
            }
        }
        else
        {
            float T1 = (MinLocation.Y - RayOrigin.Y) / RayDir.Y;
            float T2 = (MaxLocation.Y - RayOrigin.Y) / RayDir.Y;
            if (T1 > T2)
            {
                std::swap(T1, T2);
            }

            TMin = (T1 > TMin) ? T1 : TMin;
            TMax = (T2 < TMax) ? T2 : TMax;
            if (TMin > TMax)
            {
                return false;
            }
        }

        // Z축 처리
        if (FMath::Abs(RayDir.Z) < epsilon)
        {
            if (RayOrigin.Z < MinLocation.Z || RayOrigin.Z > MaxLocation.Z)
            {
                return false;
            }
        }
        else
        {
            float T1 = (MinLocation.Z - RayOrigin.Z) / RayDir.Z;
            float T2 = (MaxLocation.Z - RayOrigin.Z) / RayDir.Z;
            if (T1 > T2)
            {
                std::swap(T1, T2);
            }

            TMin = (T1 > TMin) ? T1 : TMin;
            TMax = (T2 < TMax) ? T2 : TMax;
            if (TMin > TMax)
            {
                return false;
            }
        }

        // 여기까지 왔으면 교차 구간 [tmin, tmax]가 유효하다.
        // tmax < 0 이면, 레이가 박스 뒤쪽에서 교차하므로 화면상 보기엔 교차 안 한다고 볼 수 있음
        if (TMax < 0.0f)
        {
            return false;
        }

        // outDistance = tmin이 0보다 크면 그게 레이가 처음으로 박스를 만나는 지점
        // 만약 tmin < 0 이면, 레이의 시작점이 박스 내부에 있다는 의미이므로, 거리를 0으로 처리해도 됨.
        OutDistance = (TMin >= 0.0f) ? TMin : 0.0f;

        return true;
    }
};

struct FCone
{
    FVector ConeApex; // 원뿔의 꼭짓점
    float ConeRadius; // 원뿔 밑면 반지름

    FVector ConeBaseCenter; // 원뿔 밑면 중심
    float ConeHeight; // 원뿔 높이 (Apex와 BaseCenter 간 차이)
    
    FVector4 Color;

    int ConeSegmentCount; // 원뿔 밑면 분할 수
    float pad[3];
};

struct FPrimitiveCounts
{
    int BoundingBoxCount;
    int pad;
    int ConeCount;
    int pad1;
};

#define MAX_LIGHTS 16
#define NUM_FACES 6
#define MAX_CASCADE_NUM 5

enum ELightType
{
    POINT_LIGHT = 1,
    SPOT_LIGHT = 2,
    DIRECTIONAL_LIGHT = 3,
    AMBIENT_LIGHT = 4,
    NUM_LIGHT_TYPES = 5
};

struct FMaterialConstants
{
    uint32 TextureFlag;
    FVector DiffuseColor;

    FVector SpecularColor;
    float Shininess;

    FVector EmissiveColor;
    float Transparency;

    float Metallic;
    float Roughness;
    FVector2D MaterialPadding;
};

struct FPointLightGSBuffer
{
    FMatrix World;
    FMatrix ViewProj[NUM_FACES]; // 6 : NUM_FACES
};

struct FCascadeConstantBuffer
{
    FMatrix World;
    FMatrix ViewProj[MAX_CASCADE_NUM];
    FMatrix InvViewProj[MAX_CASCADE_NUM];
    FMatrix InvProj[MAX_CASCADE_NUM];
    FVector4 CascadeSplit;

    float pad1;
    float pad2;
};

struct FShadowConstantBuffer
{
    FMatrix ShadowViewProj; // Light 광원 입장에서의 ViewProj
};

struct FObjectConstantBuffer
{
    FMatrix WorldMatrix;
    FMatrix InverseTransposedWorld;
    
    FVector4 UUIDColor;
    
    int bIsSelected;
    FVector pad;
};

struct FObjectConstantBufferInstanced
{
	FMatrix WorldMatrix;
	FMatrix InverseTransposedWorld;
};

struct FCameraConstantBuffer
{
    FMatrix ViewMatrix;
    FMatrix InvViewMatrix;
    
    FMatrix ProjectionMatrix;
    FMatrix InvProjectionMatrix;
    
    FVector ViewLocation;
    float Padding1;

    float NearClip;
    float FarClip;
    FVector2D Padding2;
};

struct FSubUVConstant
{
    FVector2D uvOffset;
    FVector2D uvScale;
};

struct FLitUnlitConstants
{
    int bIsLit; // 1 = Lit, 0 = Unlit 
    FVector pad;
};

struct FIsShadowConstants
{
    int bIsShadow;
    FVector pad;
};

struct FViewModeConstants
{
    uint32 ViewMode;
    FVector pad;
};

struct FSubMeshConstants
{
    float bIsSelectedSubMesh;
    FVector pad;
};

struct FTextureUVConstants
{
    float UOffset;
    float VOffset;
    float pad0;
    float pad1;
};

struct FLinePrimitiveBatchArgs
{
    FGridParameters GridParam;
    ID3D11Buffer* VertexBuffer = nullptr;
    int BoundingBoxCount;
    int ConeCount;
    int ConeSegmentCount;
    int OBBCount;
};

struct FViewportSize
{
    FVector2D ViewportSize;
    float Padding1;
    float Padding2;
};

struct FVertexInfo
{
    uint32_t NumVertices;
    uint32_t Stride;
    ID3D11Buffer* VertexBuffer = nullptr;
};

struct FIndexInfo
{
    uint32_t NumIndices;
    ID3D11Buffer* IndexBuffer = nullptr;
};

struct FBufferInfo
{
    FVertexInfo VertexInfo;
    FIndexInfo IndexInfo;
};

struct FFogConstants
{
    FLinearColor FogColor;
    
    float StartDistance;
    float EndDistance;    
    float FogHeight;
    float FogHeightFalloff;
    
    float FogDensity;
    float FogDistanceWeight;
    float padding1;
    float padding2;
};

struct FGammaConstants
{
    float GammaValue;
    FVector Padding;
};

struct FCPUSkinningConstants
{
    int bCPUSkinning;
    FVector Padding;
};
