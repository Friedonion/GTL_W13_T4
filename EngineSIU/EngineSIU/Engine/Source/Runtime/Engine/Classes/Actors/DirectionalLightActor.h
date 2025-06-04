#pragma once
#include "LightActor.h"
class ADirectionalLight : public ALight
{
    DECLARE_CLASS(ADirectionalLight, ALight)
    
public:
    ADirectionalLight();
    virtual ~ADirectionalLight();

public:
    void SetIntensity(float Intensity);

    float GetIntensity() const;
protected:
    UPROPERTY
    (UDirectionalLightComponent*, DirectionalLightComponent, = nullptr);

    UPROPERTY
    (UBillboardComponent*, BillboardComponent, = nullptr);
};

