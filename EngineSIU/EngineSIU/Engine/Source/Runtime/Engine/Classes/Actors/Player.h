#pragma once
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ObjectTypes.h"


class USkeletalMeshComponent;
class UCameraComponent;
class UGizmoBaseComponent;
class UGizmoArrowComponent;
class USceneComponent;
class UPrimitiveComponent;
class FEditorViewportClient;
class UStaticMeshComponent;

class AEditorPlayer : public AActor
{
    DECLARE_CLASS(AEditorPlayer, AActor)

    AEditorPlayer() = default;

    virtual void Tick(float DeltaTime) override;

    void Input();
    bool PickGizmo(FVector& RayOrigin, FEditorViewportClient* InActiveViewport);
    void ProcessGizmoIntersection(UStaticMeshComponent* Component, const FVector& PickPosition, FEditorViewportClient* InActiveViewport, bool& bIsPickedGizmo);
    void PickActor(const FVector& PickPosition);
    void AddControlMode();
    void AddCoordMode();
    void SetCoordMode(ECoordMode InMode) { CoordMode = InMode; }

private:
    static int RayIntersectsObject(const FVector& PickPosition, USceneComponent* Component, float& HitDistance, int& IntersectCount);
    void ScreenToViewSpace(int32 ScreenX, int32 ScreenY, std::shared_ptr<FEditorViewportClient> ActiveViewport, FVector& RayOrigin);
    void PickedObjControl();
    void PickedBoneControl();
    
    void ControlRotation(USceneComponent* Component, UGizmoBaseComponent* Gizmo, float DeltaX, float DeltaY);
    
    void ControlScale(USceneComponent* Component, UGizmoBaseComponent* Gizmo, float DeltaX, float DeltaY);
    FQuat ControlBoneRotation(FTransform& Component, UGizmoBaseComponent* Gizmo, float DeltaX, float DeltaY);
    FVector ControlBoneScale(FTransform& Component, UGizmoBaseComponent* Gizmo, float DeltaX, float DeltaY);
    

    bool bLeftMouseDown = false;

    POINT LastMousePos = { 0,0 };
    EControlMode ControlMode = CM_TRANSLATION;
    ECoordMode CoordMode = CDM_WORLD;
    FQuat InitialBoneRotationForGizmo;

public:
    void SetMode(EControlMode Mode) { ControlMode = Mode; }
    EControlMode GetControlMode() const { return ControlMode; }
    ECoordMode GetCoordMode() const { return CoordMode; }
};

class APlayer : public AActor
{
    DECLARE_CLASS(APlayer, AActor)

public:
    APlayer() = default;

    virtual UObject* Duplicate(UObject* InOuter) override;
    virtual void Tick(float DeltaTime) override;
};

#pragma region W10
class ASequencerPlayer : public APlayer
{
    DECLARE_CLASS(ASequencerPlayer, APlayer)

public:
    ASequencerPlayer();
    virtual ~ASequencerPlayer() override = default;

    virtual void PostSpawnInitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual UObject* Duplicate(UObject* InOuter) override;

    FName Socket = "jx_c_camera";
    USkeletalMeshComponent* SkeletalMeshComponent = nullptr;

private:
    UCameraComponent* CameraComponent = nullptr;
};
#pragma endregion
