#pragma once
#include "UnrealClient.h"
#include "Components/SceneComponent.h"
#include "LightDefine.h"

#define NUM_FACES 6

class ULightComponentBase : public USceneComponent
{
    DECLARE_CLASS(ULightComponentBase, USceneComponent)

public:
    ULightComponentBase();
    virtual ~ULightComponentBase() override = default;
    
    virtual void Initialize();
    virtual UObject* Duplicate(UObject* InOuter) override;
    
    virtual void GetProperties(TMap<FString, FString>& OutProperties) const override;
    virtual void SetProperties(const TMap<FString, FString>& InProperties) override;

    virtual void TickComponent(float DeltaTime) override;
    virtual int CheckRayIntersection(const FVector& InRayOrigin, const FVector& InRayDirection, float& OutHitDistance) const override;

    virtual void UpdateViewMatrix();
    virtual void UpdateProjectionMatrix();
    
    FMatrix GetViewMatrix(int Index = 0) const
    {
        return ViewMatrices[Index];
    }
    FMatrix GetProjectionMatrix() const
    {
        return ProjectionMatrix;
    }
    FMatrix GetViewProjectionMatrix(int Index = 0) const
    {
        return ViewMatrices[Index] * ProjectionMatrix;
    }
    
protected:
    // PointLight: 6개의 ViewMatrix를 가집니다
    TArray<FMatrix> ViewMatrices;
    FMatrix ProjectionMatrix;
    
    FBoundingBox AABB;

public:
    FBoundingBox GetBoundingBox() const {return AABB;}

public:
    // bool HasShadowMap() const { return ShadowMaps.Num() != 0; }
    //virtual TArray<FDepthStencilRHI> GetShadowMap();
    void SetShadowMapSize(const uint32 InWidth, const uint32 InHeight);
    uint32 GetShadowMapWidth() const { return ShadowMapWidth; }
    uint32 GetShadowMapHeight() const { return ShadowMapHeight; }

protected:
    uint32 ShadowMapWidth = 4096;
    uint32 ShadowMapHeight = 4096;
    bool bDirtyFlag = false;
};
