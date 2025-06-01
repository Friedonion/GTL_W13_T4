#pragma once

#include "Actors/Player.h"
#include "Components/ActorComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "UnrealEd/EditorPanel.h"
#include "Math/Rotator.h"
#include "UObject/Casts.h"

class UParticleSystemComponent;
class USkeletalMeshComponent;
class USpringArmComponent;
class UShapeComponent;
class UAmbientLightComponent;
class UDirectionalLightComponent;
class UPointLightComponent;
class USpotLightComponent;
class ULightComponentBase;
class UProjectileMovementComponent;
class UTextComponent;
class UHeightFogComponent;
class AEditorPlayer;
class UStaticMeshComponent;
class UMaterial;
class USocketComponent;

// 헬퍼 함수 예시
template<typename Getter, typename Setter>
void DrawColorProperty(const char* Label, Getter Get, Setter Set)
{
    ImGui::PushItemWidth(200.0f);
    const FLinearColor CurrentColor = Get();
    float Col[4] = { CurrentColor.R, CurrentColor.G, CurrentColor.B, CurrentColor.A };

    if (ImGui::ColorEdit4(Label, Col,
        ImGuiColorEditFlags_DisplayRGB |
        ImGuiColorEditFlags_NoSidePreview |
        ImGuiColorEditFlags_NoInputs |
        ImGuiColorEditFlags_Float))
    {
        Set(FLinearColor(Col[0], Col[1], Col[2], Col[3]));
    }
    ImGui::PopItemWidth();
}


class PropertyEditorPanel : public UEditorPanel
{
public:
    PropertyEditorPanel();

    virtual void Render() override;
    virtual void OnResize(HWND hWnd) override;

private:
    static void RGBToHSV(float R, float G, float B, float& H, float& S, float& V);
    static void HSVToRGB(float H, float S, float V, float& R, float& G, float& B);

    void RenderForSceneComponent(USceneComponent* SceneComponent, AEditorPlayer* Player) const;
    void RenderForCameraComponent(UCameraComponent* InCameraComponent);
    void RenderForPlayerActor(APlayer* InPlayerActor);
    void RenderForActor(AActor* SelectedActor, USceneComponent* TargetComponent) const;
    
    /* Static Mesh Settings */
    void RenderForStaticMesh(UStaticMeshComponent* StaticMeshComp) const;
    void RenderForSkeletalMesh(USkeletalMeshComponent* SkeletalMeshComp) const;
    void RenderForPhysicsAsset(const USkeletalMeshComponent* SkeletalMeshComp) const;
    void RenderForParticleSystem(UParticleSystemComponent* ParticleSystemComponent) const;
    void RenderForSocketComponent(USocketComponent* SocketComponent) const;

    void RenderForAmbientLightComponent(UAmbientLightComponent* AmbientLightComponent) const;
    void RenderForDirectionalLightComponent(UDirectionalLightComponent* DirectionalLightComponent) const;
    void RenderForPointLightComponent(UPointLightComponent* PointlightComponent) const;
    void RenderForSpotLightComponent(USpotLightComponent* SpotLightComponent) const;

    void RenderForLightCommon(ULightComponentBase* LightComponent) const;
    
    void RenderForProjectileMovementComponent(UProjectileMovementComponent* ProjectileComp) const;
    void RenderForTextComponent(UTextComponent* TextComponent) const;
    
    /* Materials Settings */
    void RenderForMaterial(UStaticMeshComponent* StaticMeshComp);
    void RenderMaterialView(UMaterial* Material);
    void RenderCreateMaterialView();

    void RenderForExponentialHeightFogComponent(UHeightFogComponent* FogComponent) const;

    void RenderForShapeComponent(UShapeComponent* ShapeComponent) const;
    void RenderForSpringArmComponent(USpringArmComponent* SpringArmComponent) const;
    
    template<typename T>
        requires std::derived_from<T, UActorComponent>
    T* GetTargetComponent(AActor* SelectedActor, USceneComponent* SelectedComponent);

    
private:
    float Width = 0, Height = 0;
    /* Material Property */
    int SelectedMaterialIndex = -1;
    int CurMaterialIndex = -1;
    UStaticMeshComponent* SelectedStaticMeshComp = nullptr;
    FMaterialInfo tempMaterialInfo;
    bool IsCreateMaterial = false;

    const FString TemplateFilePath = FString("LuaScripts/template.lua");
};

template <typename T> requires std::derived_from<T, UActorComponent>
T* PropertyEditorPanel::GetTargetComponent(AActor* SelectedActor, USceneComponent* SelectedComponent)
{
    T* ResultComp = nullptr;
    if (SelectedComponent != nullptr)
    {
        ResultComp = Cast<T>(SelectedComponent);
    }
    else if (SelectedActor != nullptr)
    {
        ResultComp = SelectedActor->GetComponentByClass<T>();
    }
        
    return ResultComp;
}
