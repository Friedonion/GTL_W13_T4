#include "DirectionalLightActor.h"
#include "Components/Light/DirectionalLightComponent.h"
#include "Components/BillboardComponent.h"
ADirectionalLight::ADirectionalLight()
{
    DirectionalLightComponent = AddComponent<UDirectionalLightComponent>("UDirectionalLightComponent_0");
    BillboardComponent = AddComponent<UBillboardComponent>("UBillboardComponent_0");

    RootComponent = BillboardComponent;

    BillboardComponent->SetTexture(L"Assets/Editor/Icon/S_LightDirectional.PNG");
    BillboardComponent->bIsEditorBillboard = true;

    DirectionalLightComponent->AttachToComponent(RootComponent);
}

ADirectionalLight::~ADirectionalLight()
{
}

void ADirectionalLight::SetIntensity(float Intensity)
{
    if (DirectionalLightComponent)
    {
        DirectionalLightComponent->SetIntensity(Intensity);
    }
}

float ADirectionalLight::GetIntensity() const
{
    if (DirectionalLightComponent)
    {
        return DirectionalLightComponent->GetIntensity();
    }
    return 0.0f;
}
