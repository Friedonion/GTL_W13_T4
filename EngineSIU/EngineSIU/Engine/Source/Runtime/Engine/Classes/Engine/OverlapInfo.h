
#pragma once
#include "CoreMiscDefines.h"
#include "HAL/PlatformType.h"
#include "HitResult.h"

class UPrimitiveComponent;

/** Overlap info consisting of the primitive and the body that is overlapping */
struct FOverlapInfo
{
    FOverlapInfo()
    {}

    explicit FOverlapInfo(const FHitResult& InSweepResult)
        : bFromSweep(true), OverlapInfo(InSweepResult)
    {
    }

    explicit FOverlapInfo(UPrimitiveComponent* InComponent, int32 InBodyIndex = INDEX_NONE);

    int32 GetBodyIndex() const { return OverlapInfo.Item; }

    //This function completely ignores SweepResult information. It seems that places that use this function do not care, but it still seems risky
    friend bool operator == (const FOverlapInfo& Lhs, const FOverlapInfo& Rhs) { return Lhs.OverlapInfo.Component == Rhs.OverlapInfo.Component && Lhs.OverlapInfo.Item == Rhs.OverlapInfo.Item; }
    bool bFromSweep = false;

    /** Information for both sweep and overlap queries. Different parts are valid depending on bFromSweep.
      * If bFromSweep is true then FHitResult is completely valid just like a regular sweep result.
      * If bFromSweep is false only FHitResult::Component, FHitResult::Actor, FHitResult::Item are valid as this is really just an FOverlapResult*/
    FHitResult OverlapInfo;
};

/*
 * Predicate for comparing FOverlapInfos when exact weak object pointer index/serial numbers should match, assuming one is not null and not invalid.
 * Compare to operator== for WeakObjectPtr which does both HasSameIndexAndSerialNumber *and* IsValid() checks on both pointers.
 */
struct FFastOverlapInfoCompare
{
    FFastOverlapInfoCompare(const FOverlapInfo& BaseInfo)
        : MyBaseInfo(BaseInfo)
    {
    }

    bool operator() (const FOverlapInfo& Info) const
    {
        return MyBaseInfo.OverlapInfo.Component == Info.OverlapInfo.Component
            /* && MyBaseInfo.GetBodyIndex() == Info.GetBodyIndex() */;
    }

    bool operator() (const FOverlapInfo* Info) const
    {
        return MyBaseInfo.OverlapInfo.Component == Info->OverlapInfo.Component
            /* && MyBaseInfo.GetBodyIndex() == Info->GetBodyIndex() */;
    }

private:
    const FOverlapInfo& MyBaseInfo;

};
