#pragma once
#include "CameraTypes.h"
#include "Components/SceneComponent.h"

class UCameraComponent : public USceneComponent
{
public:
    DECLARE_CLASS(UCameraComponent, USceneComponent)

    UCameraComponent() = default;

    virtual UObject* Duplicate(UObject* InOuter) override;
    virtual void InitializeComponent() override;
    virtual void TickComponent(float DeltaTime) override;
    void FollowMainPlayer();

    float ViewFOV = 70.0f;
    float NearClip = 20.f;
    float FarClip = 50000.0f;

    float DistanceBehind = 10.f;
    float CameraHeight = 15.f;
    float CurrentCameraZ = 0.f;
    float CameraZ = 0.f; //바닥에 닿을때마다 바닥 Z로 업데이트
    float CameraZOffset = 8.f; //너무 아래 보기때문에 조금 위를 향해서 보는 변수

    void SetLocationWithFInterpTo(const FVector& ToLocation);
    void SetFInterpToSpeed(float InSpeed);
    void SetLookTarget(FVector& Location);

    FVector GetLocationWithFInterpTo() const { return FInterpTargetLocation; }

    bool bFollowCustomTarget = false;
    FVector CustomTargetLocation;

    void SetFollowCustomTarget(const FVector& InLocation);
    void ResetFollowToPlayer();
    void GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView);

private:
    void ProceedFInterp(float DeltaTime);
    
    FVector FInterpTargetLocation = FVector::ZeroVector;
    FVector LookTarget = FVector::ZeroVector;
    float FInterpToSpeed = 0.8f;
};
