#pragma once
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

class USkeleton;

struct FAnimExtractContext
{
    double CurrentTime;

    bool bLooping;

    FAnimExtractContext(double InCurrentTime = 0.0, bool InbLooping = false)
        : CurrentTime(InCurrentTime)
        , bLooping(InbLooping)
    {
    }
};

class UAnimationAsset : public UObject
{
    DECLARE_CLASS(UAnimationAsset, UObject)

public:
    UAnimationAsset() = default;
    virtual ~UAnimationAsset() override = default;

    virtual float GetPlayLength() const { return 0.f; }

    void SetSkeleton(USkeleton* NewSkeleton) { Skeleton = NewSkeleton; }

    USkeleton* GetSkeleton() const { return Skeleton; }

private:
    USkeleton* Skeleton = nullptr;
};
