#pragma once
#include "Math/Rotator.h"
#include "Math/Vector.h"

namespace ECameraProjectionMode
{
    enum Type : int
    {
        Perspective,
        Orthographic
    };
}

struct FPostProcessSettings
{
    
};

/* 카메라 필요 인자 추가하시면 됩니다 */
struct FMinimalViewInfo
{
    FVector Location;
    FRotator Rotation;
    float FOV;
    float PerspectiveNearClip;
    float PerspectiveFarClip;

    /*float OrthoWidth;
    float OrthoNearClipPlane;
    float OrthoFarClipPlane;
    float AspectRatio;*/
    FMinimalViewInfo()
        : Location(0.0f, 0.0f, 0.0f)
        , Rotation(0.0f, 0.0f, 0.0f)
        , FOV(90.0f)
        , PerspectiveNearClip(0.1f)
        , PerspectiveFarClip(500000.f)
        /*, OrthoWidth(512.0f)
        , OrthoNearClipPlane(1.0f)
        , OrthoFarClipPlane(10000.0f)
        , AspectRatio(1.33333333f)*/
    {
    }

    bool Equals(const FMinimalViewInfo& Other) const;
    void BlendViewInfo(FMinimalViewInfo& OtherInfo, float OtherWeight);
};

inline bool FMinimalViewInfo::Equals(const FMinimalViewInfo& Other) const
{
    return 
        Location == Other.Location &&
        Rotation == Other.Rotation &&
        FOV == Other.FOV &&
        PerspectiveNearClip == Other.PerspectiveNearClip &&
        PerspectiveFarClip == Other.PerspectiveFarClip;
}

inline void FMinimalViewInfo::BlendViewInfo(FMinimalViewInfo& OtherInfo, float OtherWeight)
{
    Location = FMath::Lerp(Location, OtherInfo.Location, OtherWeight);

    const FRotator DeltaAng = (OtherInfo.Rotation - Rotation).GetNormalized();
    Rotation = Rotation + OtherWeight * DeltaAng;

    FOV = FMath::Lerp(FOV, OtherInfo.FOV, OtherWeight);
}

