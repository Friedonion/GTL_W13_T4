#pragma once
#include "LightComponent.h"
#include "UObject/ObjectMacros.h"
#include "LightDefine.h"

class UDirectionalLightComponent : public ULightComponentBase
{
    DECLARE_CLASS(UDirectionalLightComponent, ULightComponentBase)

public:
    UDirectionalLightComponent();
    virtual ~UDirectionalLightComponent() override = default;

    virtual UObject* Duplicate(UObject* InOuter) override;
    
    virtual void GetProperties(TMap<FString, FString>& OutProperties) const override;
    virtual void SetProperties(const TMap<FString, FString>& InProperties) override;
    FVector GetDirection();
    float GetShadowNearPlane() const;

    const FDirectionalLightInfo& GetDirectionalLightInfo() const;
    void SetDirectionalLightInfo(const FDirectionalLightInfo& InDirectionalLightInfo);

    float GetIntensity() const;
    void SetIntensity(float InIntensity);

    bool GetCastShadows() const { return DirectionalLightInfo.CastShadows; }
    void SetCastShadows(bool InCastShadows) { DirectionalLightInfo.CastShadows = InCastShadows; }

    FLinearColor GetLightColor() const;
    void SetLightColor(const FLinearColor& InColor);

    void UpdateViewMatrix(FVector TargetPosition);
    void UpdateViewMatrix() override;
    void UpdateProjectionMatrix() override;
    float GetShadowFrustumWidth() const;

private:
    UPROPERTY(EditAnywhere, FDirectionalLightInfo, DirectionalLightInfo, );

public:
    UPROPERTY(
        EditAnywhere,
        float,
        ShadowNearPlane,
        = 1.f
    )

    UPROPERTY(
        EditAnywhere,
        float,
        ShadowFarPlane,
        = 10000.0f
    )

    UPROPERTY(EditAnywhere, float, Param1, = 0.0f)
    UPROPERTY(EditAnywhere, float, Param2, = 0.0f)
    UPROPERTY(EditAnywhere, float, Param3, = 0.0f)
    UPROPERTY(EditAnywhere, float, Param4, = 0.0f)
    UPROPERTY(EditAnywhere, float, Param5, = 0.0f)
    UPROPERTY(EditAnywhere, float, Param6, = 0.0f)
    UPROPERTY(EditAnywhere, float, Param7, = 0.0f)
    UPROPERTY(EditAnywhere, float, Param8, = 0.0f)


    UPROPERTY_WITH_FLAGS(EditAnywhere, bool, bOverride, = false)
    UPROPERTY(EditAnywhere, float, OverrideHeight, = 1000.0f)
};

