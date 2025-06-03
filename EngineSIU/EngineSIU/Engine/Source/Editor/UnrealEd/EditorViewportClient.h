#pragma once
#include <sstream>

#include "Define.h"
#include "Container/Map.h"
#include "ViewportClient.h"
#include "EngineLoop.h"
#include "EngineBaseTypes.h"

#define  MIN_ORTHOZOOM (1.0)  // 2D ortho viewport zoom >= MIN_ORTHOZOOM
#define MAX_ORTHOZOOM (1e25)

struct FPointerEvent;
enum class EViewScreenLocation : uint8;
class FViewportResource;
class ATransformGizmo;
class USceneComponent;
struct FMinimalViewInfo;

struct FViewportCamera
{
public:
    FViewportCamera() = default;

    /** Sets the transform's location */
    void SetLocation(const FVector& Position)
    {
        ViewLocation = Position;
    }

    /** Sets the transform's rotation */
    void SetRotation(const FVector& Rotation)
    {
        ViewRotation = Rotation;
    }

    /** Sets the location to look at during orbit */
    void SetLookAt(const FVector& InLookAt)
    {
        LookAt = InLookAt;
    }

    /** Set the ortho zoom amount */
    void SetOrthoZoom(float InOrthoZoom)
    {
        assert(InOrthoZoom >= MIN_ORTHOZOOM && InOrthoZoom <= MAX_ORTHOZOOM);
        OrthoZoom = InOrthoZoom;
    }

    /** Check if transition curve is playing. */
    /*    bool IsPlaying();*/

    /** @return The transform's location */
    FORCEINLINE const FVector& GetLocation() const { return ViewLocation; }

    /** @return The transform's rotation */
    FORCEINLINE const FVector& GetRotation() const { return ViewRotation; }

    /** @return The look at point for orbiting */
    FORCEINLINE const FVector& GetLookAt() const { return LookAt; }

    /** @return The ortho zoom amount */
    FORCEINLINE float GetOrthoZoom() const { return OrthoZoom; }

    FVector GetForwardVector() const;
    FVector GetRightVector() const;
    FVector GetUpVector() const;

public:
    /** Current viewport Position. */
    FVector ViewLocation;
    /** Current Viewport orientation; valid only for perspective projections. */
    FVector ViewRotation;
    FVector DesiredLocation;
    /** When orbiting, the point we are looking at */
    FVector LookAt;
    /** Viewport start location when animating to another location */
    FVector StartLocation;
    /** Ortho zoom amount */
    float OrthoZoom;
};

class FEditorViewportClient : public FViewportClient
{
public:
    FEditorViewportClient();
    virtual ~FEditorViewportClient() override;

    virtual void Draw(FViewport* Viewport) override;
    virtual UWorld* GetWorld() const override { return nullptr; }
    void Initialize(EViewScreenLocation InViewportIndex, const FRect& InRect);
    void Tick(float DeltaTime);
    void Release() const;

    void Input();
    void UpdateEditorCameraMovement(float DeltaTime);
    void InputKey(const FKeyEvent& InKeyEvent);
    void MouseMove(const FPointerEvent& InMouseEvent);
    void ResizeViewport(FRect Top, FRect Bottom, FRect Left, FRect Right);

    bool IsSelected(const FVector2D& InPoint) const;

    /**
     * ScreenPos를 World Space로 Deprojection 합니다.
     * @param ScreenPos Deproject할 스크린 좌표
     * @param OutWorldOrigin Origin Vector (World Space)
     * @param OutWorldDir Direction Vector (World Space)
     */
    void DeprojectFVector2D(const FVector2D& ScreenPos, FVector& OutWorldOrigin, FVector& OutWorldDir) const;

protected:
    /** Camera speed setting */
    int32 CameraSpeedSetting = 1;
    /** Camera speed scalar */
    float CameraSpeed = 1.0f;
    float CameraSpeedMultiplier = 10.0f;
    float GridSize;

    void GetViewInfo(FMinimalViewInfo& OutViewInfo) const;

public:
    FViewport* Viewport;
    int32 ViewportIndex;
    FViewport* GetViewport() const { return Viewport; }
    
    const D3D11_VIEWPORT& GetD3DViewport() const;

    FViewportResource* GetViewportResource();

private:
    FViewportResource* ViewportResourceCache = nullptr;

public:
    FViewportCamera* GetPerspectiveCamera() { return &PerspectiveCamera; }

    //카메라
    /** Viewport camera transform data for perspective viewports */
    FViewportCamera PerspectiveCamera;
    FViewportCamera OrthogonalCamera;
    
    // 카메라 정보 
    float ViewFOV = 90.0f;
    float AspectRatio;
    float NearClip = 0.1f;
    float FarClip = 500000.0f;
    
    float F_Stop = 2.8f;
    float SensorWidth = 24.576f; // mm
    float FocalDistance = 0.f; // cm

    float GetFocalLength() const
    {
        const float FovRad = FMath::DegreesToRadians(ViewFOV);
        const float FocalLength = SensorWidth / (2.f * FMath::Tan(FovRad / 2.0f));
        return FocalLength;
    }
    
    static FVector Pivot;
    static float OrthoSize;
    ELevelViewportType ViewportType;
    uint64 ShowFlag;
    EViewModeIndex ViewMode;

    FMatrix View;
    FMatrix Projection;

    //Camera Movement
    void CameraMoveForward(float InValue);
    void CameraMoveRight(float InValue);
    void CameraMoveUp(float InValue);
    void CameraRotateYaw(float InValue);
    void CameraRotatePitch(float InValue);
    void PivotMoveRight(float InValue) const;
    void PivotMoveUp(float InValue) const;

    FMatrix& GetViewMatrix() { return View; }
    FMatrix& GetProjectionMatrix() { return Projection; }
    void UpdateViewMatrix();
    void UpdateProjectionMatrix();

    bool IsOrthographic() const;
    bool IsPerspective() const;

    FVector GetCameraLocation() const;

    float GetCameraFOV() const;

    float GetCameraNearClip() const;
    float GetCameraFarClip() const;
    
    ELevelViewportType GetViewportType() const;
    void SetViewportType(ELevelViewportType InViewportType);
    
    void UpdateOrthoCameraLoc();
    
    EViewModeIndex GetViewMode() const { return ViewMode; }
    void SetViewMode(EViewModeIndex InViewMode) { ViewMode = InViewMode; }
    
    uint64 GetShowFlag() const { return ShowFlag; }
    void SetShowFlag(uint64 InShowFlag) { ShowFlag = InShowFlag; }
    
    bool GetIsOnRBMouseClick() const { return bRightMouseDown; }

    // Flag Test Code
    static void SetOthoSize(float InValue);

private: // Input
    POINT PrevMousePos;
    bool bRightMouseDown = false;

    // 카메라 움직임에 사용될 키를 임시로 저장해서 사용할 예정
    TSet<EKeys::Type> PressedKeys;

public:
    void LoadConfig(const TMap<FString, FString>& Config);
    void SaveConfig(TMap<FString, FString>& Config) const;

private:
    static TMap<FString, FString> ReadIniFile(const FString& FilePath);
    static void WriteIniFile(const FString& FilePath, const TMap<FString, FString>& Config);

public:
    void SetCameraSpeedSetting(const int32& value) { CameraSpeedSetting = value; }
    int32 GetCameraSpeedSetting() const { return CameraSpeedSetting; }
    void SetGridSize(const float& value) { GridSize = value; }
    float GetGridSize() const { return GridSize; }
    float GetCameraSpeedScalar() const { return CameraSpeed; }
    void SetCameraSpeed(float InValue);

private:
    template <typename T>
    T GetValueFromConfig(const TMap<FString, FString>& config, const FString& key, T defaultValue)
    {
        if (const FString* Value = config.Find(key))
        {
            std::istringstream iss(**Value);
            T value;
            if (iss >> value)
            {
                return value;
            }
        }
        return defaultValue;
    }

public:
    // Gizmo
    // void SetGizmoActor(ATransformGizmo* gizmo) { GizmoActor = gizmo; }
    ATransformGizmo* GetGizmoActor() const { return GizmoActor; }

    void SetPickedGizmoComponent(USceneComponent* component) { PickedGizmoComponent = component; }
    USceneComponent* GetPickedGizmoComponent() const { return PickedGizmoComponent; }

    void SetShowGizmo(bool bShow) { bShowGizmo = bShow; }
    bool IsShowGizmo() const { return bShowGizmo; }

private:
    ATransformGizmo* GizmoActor = nullptr;
    USceneComponent* PickedGizmoComponent = nullptr;
    bool bShowGizmo = true;
};
