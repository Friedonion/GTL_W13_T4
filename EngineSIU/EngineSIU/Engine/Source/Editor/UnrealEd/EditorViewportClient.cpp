#include "EditorViewportClient.h"
#include "fstream"
#include "sstream"
#include "ostream"
#include "Math/JungleMath.h"
#include "UnrealClient.h"
#include "WindowsCursor.h"
#include "World/World.h"
#include "GameFramework/Actor.h"
#include "Engine/EditorEngine.h"

#include "UObject/ObjectFactory.h"
#include "BaseGizmos/TransformGizmo.h"
#include "Camera/CameraComponent.h"
#include "LevelEditor/SLevelEditor.h"
#include "SlateCore/Input/Events.h"
#include "Actors/EditorPlayer.h"

FVector FEditorViewportClient::Pivot = FVector(0.0f, 0.0f, 0.0f);
float FEditorViewportClient::OrthoSize = 10.0f;

FEditorViewportClient::FEditorViewportClient()
    : Viewport(nullptr)
    , ViewportType(LVT_Perspective)
    , ShowFlag(ULLONG_MAX)
    , ViewMode(EViewModeIndex::VMI_Lit_BlinnPhong)
{
}

FEditorViewportClient::~FEditorViewportClient()
{
    Release();

    ViewportResourceCache = nullptr;
}

void FEditorViewportClient::Draw(FViewport* Viewport)
{
}

void FEditorViewportClient::Initialize(EViewScreenLocation InViewportIndex, const FRect& InRect)
{
    ViewportIndex = static_cast<int32>(InViewportIndex);
    
    PerspectiveCamera.SetLocation(FVector(8.0f, 8.0f, 8.f));
    PerspectiveCamera.SetRotation(FVector(0.0f, 45.0f, -135.0f));
    
    Viewport = new FViewport(InViewportIndex);
    Viewport->Initialize(InRect);

    GizmoActor = FObjectFactory::ConstructObject<ATransformGizmo>(GEngine); // TODO : EditorEngine 외의 다른 Engine 형태가 추가되면 GEngine 대신 다른 방식으로 넣어주어야 함.
    GizmoActor->Initialize(this);
}

void FEditorViewportClient::Tick(const float DeltaTime)
{
    if (GEngine->ActiveWorld->WorldType != EWorldType::PIE)
    {
        UpdateEditorCameraMovement(DeltaTime);
    }
    UpdateViewMatrix();
    UpdateProjectionMatrix();
    GizmoActor->Tick(DeltaTime);
}

void FEditorViewportClient::Release() const
{
    delete Viewport;
}

void FEditorViewportClient::UpdateEditorCameraMovement(const float DeltaTime)
{
    if (PressedKeys.Contains(EKeys::A))
    {
        CameraMoveRight(-CameraSpeedMultiplier * DeltaTime);
    }

    if (PressedKeys.Contains(EKeys::D))
    {
        CameraMoveRight(CameraSpeedMultiplier * DeltaTime);
    }

    if (PressedKeys.Contains(EKeys::W))
    {
        CameraMoveForward(CameraSpeedMultiplier * DeltaTime);
    }

    if (PressedKeys.Contains(EKeys::S))
    {
        CameraMoveForward(-CameraSpeedMultiplier * DeltaTime);
    }

    if (PressedKeys.Contains(EKeys::E))
    {
        CameraMoveUp(CameraSpeedMultiplier * DeltaTime);
    }

    if (PressedKeys.Contains(EKeys::Q))
    {
        CameraMoveUp(-CameraSpeedMultiplier * DeltaTime);
    }
}

void FEditorViewportClient::InputKey(const FKeyEvent& InKeyEvent)
{
    if (GetKeyState(VK_RBUTTON) & 0x8000)
    {
        switch (InKeyEvent.GetCharacter())
        {
        case 'A':
        {
            if (InKeyEvent.GetInputEvent() == IE_Pressed)
            {
                PressedKeys.Add(EKeys::A);
            }
            else if (InKeyEvent.GetInputEvent() == IE_Released)
            {
                PressedKeys.Remove(EKeys::A);
            }
            break;
        }
        case 'D':
        {
            if (InKeyEvent.GetInputEvent() == IE_Pressed)
            {
                PressedKeys.Add(EKeys::D);
            }
            else if (InKeyEvent.GetInputEvent() == IE_Released)
            {
                PressedKeys.Remove(EKeys::D);
            }
            break;
        }
        case 'W':
        {
            if (InKeyEvent.GetInputEvent() == IE_Pressed)
            {
                PressedKeys.Add(EKeys::W);
            }
            else if (InKeyEvent.GetInputEvent() == IE_Released)
            {
                PressedKeys.Remove(EKeys::W);
            }
            break;
        }
        case 'S':
        {
            if (InKeyEvent.GetInputEvent() == IE_Pressed)
            {
                PressedKeys.Add(EKeys::S);
            }
            else if (InKeyEvent.GetInputEvent() == IE_Released)
            {
                PressedKeys.Remove(EKeys::S);
            }
            break;
        }
        case 'E':
        {
            if (InKeyEvent.GetInputEvent() == IE_Pressed)
            {
                PressedKeys.Add(EKeys::E);
            }
            else if (InKeyEvent.GetInputEvent() == IE_Released)
            {
                PressedKeys.Remove(EKeys::E);
            }
            break;
        }
        case 'Q':
        {
            if (InKeyEvent.GetInputEvent() == IE_Pressed)
            {
                PressedKeys.Add(EKeys::Q);
            }
            else if (InKeyEvent.GetInputEvent() == IE_Released)
            {
                PressedKeys.Remove(EKeys::Q);
            }
            break;
        }
        default:
            break;
        }
    }
    else
    {
        AEditorPlayer* EdPlayer = CastChecked<UEditorEngine>(GEngine)->GetEditorPlayer();
        switch (InKeyEvent.GetCharacter())
        {
        case 'W':
        {
            EdPlayer->SetMode(CM_TRANSLATION);
            break;
        }
        case 'E':
        {
            EdPlayer->SetMode(CM_ROTATION);
            break;
        }
        case 'R':
        {
            EdPlayer->SetMode(CM_SCALE);
            break;
        }
        default:
            break;
        }
        PressedKeys.Empty();
    }


    // 일반적인 단일 키 이벤트
    if (InKeyEvent.GetInputEvent() == IE_Pressed)
    {
        switch (InKeyEvent.GetCharacter())
        {
        case 'F':
        {
            UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);
            USceneComponent* SelectedComponent = Engine->GetSelectedComponent();
            AActor* SelectedActor = Engine->GetSelectedActor();

            USceneComponent* TargetComponent = nullptr;

            if (SelectedComponent != nullptr)
            {
                TargetComponent = SelectedComponent;
            }
            else if (SelectedActor != nullptr)
            {
                TargetComponent = SelectedActor->GetRootComponent();
            }
            if (TargetComponent != nullptr)
            {
                if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(TargetComponent))
                {
                    FViewportCamera& ViewTransform = PerspectiveCamera;
                    float FOV = ViewFOV;

                    // 로컬 바운딩 박스
                    FBoundingBox Box = Primitive->GetBoundingBox();
                    FVector LocalCenter = (Box.MinLocation + Box.MaxLocation) * 0.5f;
                    FVector LocalExtents = (Box.MaxLocation - Box.MinLocation) * 0.5f;
                    float Radius = LocalExtents.Length();


                    FMatrix ComponentToWorld = Primitive->GetWorldMatrix();
                    FVector WorldCenter = ComponentToWorld.TransformPosition(LocalCenter);

                    // FOV 기반 거리 계산
                    float VerticalFOV = FMath::DegreesToRadians(FOV);
                    float Distance = FMath::Max(Radius / FMath::Tan(VerticalFOV * 0.5f), 10.0f);

                    // 카메라 위치 설정
                    ViewTransform.SetLocation(WorldCenter - ViewTransform.GetForwardVector() * Distance);
                }

                 else
                {
                    FViewportCamera& ViewTransform = PerspectiveCamera;
                    ViewTransform.SetLocation(
                        // TODO: 10.0f 대신, 정점의 min, max의 거리를 구해서 하면 좋을 듯
                        TargetComponent->GetComponentLocation() - (ViewTransform.GetForwardVector() * 10.0f)
                    );
                }
            }
            break;
        }
        case 'M':
        {
            FEngineLoop::GraphicDevice.Resize(GEngineLoop.AppWnd);
            SLevelEditor* LevelEd = GEngineLoop.GetLevelEditor();
            LevelEd->SetEnableMultiViewport(!LevelEd->IsMultiViewport());
            break;
        }
        default:
            break;
        }

        // Virtual Key
        UEditorEngine* EdEngine = CastChecked<UEditorEngine>(GEngine);
        switch (InKeyEvent.GetKeyCode())
        {
        case VK_DELETE:
        {
            if (GEngine->ActiveWorld->WorldType == EWorldType::SkeletalViewer)
                return;
            UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);
            if (Engine)
            {
                USceneComponent* SelectedComponent = Engine->GetSelectedComponent();
                AActor* SelectedActor = Engine->GetSelectedActor();

                if (SelectedComponent)
                {
                    AActor* Owner = SelectedComponent->GetOwner();

                    if (Owner && Owner->GetRootComponent() != SelectedComponent)
                    {
                        UE_LOG(ELogLevel::Display, "Delete Component - %s", *SelectedComponent->GetName());
                        Engine->DeselectComponent(SelectedComponent);
                        SelectedComponent->DestroyComponent();
                    }
                    else if (SelectedActor)
                    {
                        UE_LOG(ELogLevel::Display, "Delete Component - %s", *SelectedActor->GetName());
                        Engine->DeselectActor(SelectedActor);
                        Engine->DeselectComponent(SelectedComponent);
                        Engine->ActiveWorld->DestroyActor(SelectedActor);
                    }
                }
                else if (SelectedActor)
                {
                    UE_LOG(ELogLevel::Display, "Delete Component - %s", *SelectedActor->GetName());
                    Engine->DeselectActor(SelectedActor);
                    Engine->DeselectComponent(SelectedComponent);
                    Engine->ActiveWorld->DestroyActor(SelectedActor);
                }
            }
            break;
        }
        case VK_SPACE:
        {
            EdEngine->GetEditorPlayer()->AddControlMode();
            break;
        }
        default:
            break;
        }
    }
    return;
}

void FEditorViewportClient::MouseMove(const FPointerEvent& InMouseEvent)
{
    const auto& [DeltaX, DeltaY] = InMouseEvent.GetCursorDelta();

    // Yaw(좌우 회전) 및 Pitch(상하 회전) 값 변경
    if (IsPerspective())
    {
        CameraRotateYaw(DeltaX * 0.1f);  // X 이동에 따라 좌우 회전
        CameraRotatePitch(DeltaY * 0.1f);  // Y 이동에 따라 상하 회전
    }
    else
    {
        PivotMoveRight(DeltaX);
        PivotMoveUp(DeltaY);
    }
}

void FEditorViewportClient::ResizeViewport(FRect Top, FRect Bottom, FRect Left, FRect Right)
{
    if (Viewport)
    {
        Viewport->ResizeViewport(Top, Bottom, Left, Right);
    }
    else
    {
        UE_LOG(ELogLevel::Error, "Viewport is nullptr");
    }
    UpdateProjectionMatrix();
    UpdateViewMatrix();
}

bool FEditorViewportClient::IsSelected(const FVector2D& InPoint) const
{
    return GetViewport()->bIsHovered(InPoint);
}

void FEditorViewportClient::DeprojectFVector2D(const FVector2D& ScreenPos, FVector& OutWorldOrigin, FVector& OutWorldDir) const
{
    const float TopLeftX = Viewport->GetD3DViewport().TopLeftX;
    const float TopLeftY = Viewport->GetD3DViewport().TopLeftY;
    const float Width = Viewport->GetD3DViewport().Width;
    const float Height = Viewport->GetD3DViewport().Height;

    // 뷰포트가 유효한 위치에 있는지?
    assert(0.0f <= Width && 0.0f <= Height);

    // 뷰포트의 NDC 위치
    const FVector2D NDCPos = {
        ((ScreenPos.X - TopLeftX) / Width * 2.0f) - 1.0f,
        1.0f - ((ScreenPos.Y - TopLeftY) / Height * 2.0f)
    };

    FVector RayOrigin = { NDCPos.X, NDCPos.Y, 0.0f};
    FVector RayEnd = { NDCPos.X, NDCPos.Y, 1.0f};

    // 스크린 좌표계에서 월드 좌표계로 변환
    const FMatrix InvProjView = FMatrix::Inverse(Projection) * FMatrix::Inverse(View);
    RayOrigin = InvProjView.TransformPosition(RayOrigin);
    RayEnd = InvProjView.TransformPosition(RayEnd);

    OutWorldOrigin = RayOrigin;
    OutWorldDir = (RayEnd - RayOrigin).GetSafeNormal();
}

void FEditorViewportClient::GetViewInfo(FMinimalViewInfo& OutViewInfo) const
{
    bool bGotViewInfo = false;
        
    OutViewInfo = FMinimalViewInfo();
    if (APlayerController* PC = GEngine->ActiveWorld->GetPlayerController())
    {
        if (APlayerCameraManager* PCM = PC->PlayerCameraManager)
        {
            if (PCM->PendingViewTarget.Target != nullptr)
            {
                OutViewInfo = PCM->LastFrameViewTarget.POV;
            }else
            {
                OutViewInfo = PCM->ViewTarget.POV;
            }
            bGotViewInfo = true;
        }
    }

    if (!bGotViewInfo)
    {
        OutViewInfo = FMinimalViewInfo();
    }
}


const D3D11_VIEWPORT& FEditorViewportClient::GetD3DViewport() const
{
    return Viewport->GetD3DViewport();
}

FViewportResource* FEditorViewportClient::GetViewportResource()
{
    if (!ViewportResourceCache)
    {
        ViewportResourceCache = Viewport->GetViewportResource();
    }
    return ViewportResourceCache;
}

void FEditorViewportClient::CameraMoveForward(const float InValue)
{
    if (IsPerspective())
    {
        FVector CurCameraLoc = PerspectiveCamera.GetLocation();
        CurCameraLoc = CurCameraLoc + PerspectiveCamera.GetForwardVector() * GetCameraSpeedScalar() * InValue;
        PerspectiveCamera.SetLocation(CurCameraLoc);
    }
    else
    {
        Pivot.X += InValue * 0.1f;
    }
}

void FEditorViewportClient::CameraMoveRight(const float InValue)
{
    if (IsPerspective())
    {
        FVector CurCameraLoc = PerspectiveCamera.GetLocation();
        CurCameraLoc = CurCameraLoc + PerspectiveCamera.GetRightVector() * GetCameraSpeedScalar() * InValue;
        PerspectiveCamera.SetLocation(CurCameraLoc);
    }
    else
    {
        Pivot.Y += InValue * 0.1f;
    }
}

void FEditorViewportClient::CameraMoveUp(const float InValue)
{
    if (IsPerspective())
    {
        FVector CurCameraLoc = PerspectiveCamera.GetLocation();
        CurCameraLoc.Z = CurCameraLoc.Z + GetCameraSpeedScalar() * InValue;
        PerspectiveCamera.SetLocation(CurCameraLoc);
    }
    else
    {
        Pivot.Z += InValue * 0.1f;
    }
}

void FEditorViewportClient::CameraRotateYaw(const float InValue)
{
    FVector CurCameraRot = PerspectiveCamera.GetRotation();
    CurCameraRot.Z += InValue ;
    PerspectiveCamera.SetRotation(CurCameraRot);
}

void FEditorViewportClient::CameraRotatePitch(const float InValue)
{
    FVector CurCameraRot = PerspectiveCamera.GetRotation();
    CurCameraRot.Y = FMath::Clamp(CurCameraRot.Y + InValue, -89.f, 89.f);
    PerspectiveCamera.SetRotation(CurCameraRot);
}

void FEditorViewportClient::PivotMoveRight(const float InValue) const
{
    Pivot = Pivot + OrthogonalCamera.GetRightVector() * InValue * -0.05f;
}

void FEditorViewportClient::PivotMoveUp(const float InValue) const
{
    Pivot = Pivot + OrthogonalCamera.GetUpVector() * InValue * 0.05f;
}

void FEditorViewportClient::UpdateViewMatrix()
{
    if (GEngine && GEngine->ActiveWorld->WorldType == EWorldType::PIE)
    {
        FMinimalViewInfo ViewInfo;
        GetViewInfo(ViewInfo);

        FMatrix RotationMatrix = ViewInfo.Rotation.ToMatrix();
        FVector FinalUp = FMatrix::TransformVector(FVector::UpVector, RotationMatrix);
        
        View = JungleMath::CreateViewMatrix(
            ViewInfo.Location,
            ViewInfo.Location + ViewInfo.Rotation.ToVector(),
            FinalUp
        );
    }
    else
    {
        if (IsPerspective())
        {
            View = JungleMath::CreateViewMatrix(PerspectiveCamera.GetLocation(),
                PerspectiveCamera.GetLocation() + PerspectiveCamera.GetForwardVector(),
                FVector{ 0.0f,0.0f, 1.0f }
            );
        }
        else 
        {
            UpdateOrthoCameraLoc();
            if (ViewportType == LVT_OrthoXY || ViewportType == LVT_OrthoNegativeXY)
            {
                View = JungleMath::CreateViewMatrix(OrthogonalCamera.GetLocation(),
                    Pivot, FVector(0.0f, -1.0f, 0.0f)
                );
            }
            else
            {
                View = JungleMath::CreateViewMatrix(OrthogonalCamera.GetLocation(),
                    Pivot, FVector(0.0f, 0.0f, 1.0f)
                );
            }
        }
    }
}

void FEditorViewportClient::UpdateProjectionMatrix()
{
    AspectRatio = GetViewport()->GetD3DViewport().Width / GetViewport()->GetD3DViewport().Height;

    if (GEngine && GEngine->ActiveWorld->WorldType == EWorldType::PIE)
    {
        FMinimalViewInfo ViewInfo;
        GetViewInfo(ViewInfo);
        
        Projection = JungleMath::CreateProjectionMatrix(
            FMath::DegreesToRadians(ViewInfo.FOV),
            AspectRatio,
            ViewInfo.PerspectiveNearClip,
            ViewInfo.PerspectiveFarClip
        );
    }else
    {
        if (IsPerspective())
        {
            Projection = JungleMath::CreateProjectionMatrix(
                FMath::DegreesToRadians(ViewFOV),
                AspectRatio,
                NearClip,
                FarClip
            );
        }
        else
        {
            // 오쏘그래픽 너비는 줌 값과 가로세로 비율에 따라 결정됩니다.
            const float OrthoWidth = OrthoSize * AspectRatio;
            const float OrthoHeight = OrthoSize;

            // 오쏘그래픽 투영 행렬 생성 (nearPlane, farPlane 은 기존 값 사용)
            Projection = JungleMath::CreateOrthoProjectionMatrix(
                OrthoWidth,
                OrthoHeight,
                NearClip,
                FarClip
            );
        }
    }
}

bool FEditorViewportClient::IsOrthographic() const
{
    return !IsPerspective();
}

bool FEditorViewportClient::IsPerspective() const
{
    return (GetViewportType() == LVT_Perspective);
}

FVector FEditorViewportClient::GetCameraLocation() const
{
    if (GEngine && GEngine->ActiveWorld->WorldType == EWorldType::PIE)
    {
        FMinimalViewInfo ViewInfo;
        GetViewInfo(ViewInfo);

        return ViewInfo.Location;
    }

    if (IsPerspective())
    {
        return PerspectiveCamera.GetLocation();
    }
    return OrthogonalCamera.GetLocation();
}

float FEditorViewportClient::GetCameraFOV() const
{
    if (GEngine && GEngine->ActiveWorld->WorldType == EWorldType::PIE)
    {
        FMinimalViewInfo ViewInfo;
        GetViewInfo(ViewInfo);

        return ViewInfo.FOV;
    }
    
    return ViewFOV;
}

float FEditorViewportClient::GetCameraNearClip() const
{
    if (GEngine && GEngine->ActiveWorld->WorldType == EWorldType::PIE)
    {
        FMinimalViewInfo ViewInfo;
        GetViewInfo(ViewInfo);

        return ViewInfo.PerspectiveNearClip;
    }
    
    return NearClip;
}

float FEditorViewportClient::GetCameraFarClip() const
{
    if (GEngine && GEngine->ActiveWorld->WorldType == EWorldType::PIE)
    {
        FMinimalViewInfo ViewInfo;
        GetViewInfo(ViewInfo);

        return ViewInfo.PerspectiveFarClip;
    }
    
    return FarClip;
}

ELevelViewportType FEditorViewportClient::GetViewportType() const
{
    ELevelViewportType EffectiveViewportType = ViewportType;
    if (EffectiveViewportType == LVT_None)
    {
        EffectiveViewportType = LVT_Perspective;
    }
    return EffectiveViewportType;
}

void FEditorViewportClient::SetViewportType(ELevelViewportType InViewportType)
{
    ViewportType = InViewportType;
}

void FEditorViewportClient::UpdateOrthoCameraLoc()
{
    switch (ViewportType)
    {
    case LVT_OrthoXY: // Top
        OrthogonalCamera.SetLocation(Pivot + FVector::UpVector * FarClip * 0.5f);
        OrthogonalCamera.SetRotation(FVector(0.0f, 90.0f, -90.0f));
        break;
    case LVT_OrthoXZ: // Front
        OrthogonalCamera.SetLocation(Pivot + FVector::ForwardVector * FarClip * 0.5f);
        OrthogonalCamera.SetRotation(FVector(0.0f, 0.0f, 180.0f));
        break;
    case LVT_OrthoYZ: // Left
        OrthogonalCamera.SetLocation(Pivot + FVector::RightVector * FarClip * 0.5f);
        OrthogonalCamera.SetRotation(FVector(0.0f, 0.0f, 270.0f));
        break;
    case LVT_OrthoNegativeXY: // Bottom
        OrthogonalCamera.SetLocation(Pivot + FVector::UpVector * -1.0f * FarClip * 0.5f);
        OrthogonalCamera.SetRotation(FVector(0.0f, -90.0f, 90.0f));
        break;
    case LVT_OrthoNegativeXZ: // Back
        OrthogonalCamera.SetLocation(Pivot + FVector::ForwardVector * -1.0f * FarClip * 0.5f);
        OrthogonalCamera.SetRotation(FVector(0.0f, 0.0f, 0.0f));
        break;
    case LVT_OrthoNegativeYZ: // Right
        OrthogonalCamera.SetLocation(Pivot + FVector::RightVector * -1.0f * FarClip * 0.5f);
        OrthogonalCamera.SetRotation(FVector(0.0f, 0.0f, 90.0f));
        break;
    case LVT_None:
    case LVT_Perspective:
    case LVT_MAX:
    default:
        break;
    }
}

void FEditorViewportClient::SetOthoSize(const float InValue)
{
    OrthoSize += InValue;
    OrthoSize = FMath::Max(OrthoSize, 0.1f);
}

void FEditorViewportClient::LoadConfig(const TMap<FString, FString>& Config)
{
    FString ViewportNum = std::to_string(ViewportIndex);
    CameraSpeedSetting = GetValueFromConfig(Config, "CameraSpeedSetting" + ViewportNum, 1);
    CameraSpeed = GetValueFromConfig(Config, "CameraSpeedScalar" + ViewportNum, 1.0f);
    GridSize = GetValueFromConfig(Config, "GridSize"+ ViewportNum, 10.0f);
    PerspectiveCamera.ViewLocation.X = GetValueFromConfig(Config, "PerspectiveCameraLocX" + ViewportNum, 0.0f);
    PerspectiveCamera.ViewLocation.Y = GetValueFromConfig(Config, "PerspectiveCameraLocY" + ViewportNum, 0.0f);
    PerspectiveCamera.ViewLocation.Z = GetValueFromConfig(Config, "PerspectiveCameraLocZ" + ViewportNum, 0.0f);
    PerspectiveCamera.ViewRotation.X = GetValueFromConfig(Config, "PerspectiveCameraRotX" + ViewportNum, 0.0f);
    PerspectiveCamera.ViewRotation.Y = GetValueFromConfig(Config, "PerspectiveCameraRotY" + ViewportNum, 0.0f);
    PerspectiveCamera.ViewRotation.Z = GetValueFromConfig(Config, "PerspectiveCameraRotZ" + ViewportNum, 0.0f);
    ShowFlag = GetValueFromConfig(Config, "ShowFlag" + ViewportNum, ULLONG_MAX);
    ViewMode = static_cast<EViewModeIndex>(GetValueFromConfig(Config, "ViewMode" + ViewportNum, 0));
    ViewportType = static_cast<ELevelViewportType>(GetValueFromConfig(Config, "ViewportType" + ViewportNum, 3));
}

void FEditorViewportClient::SaveConfig(TMap<FString, FString>& Config) const
{
    const FString ViewportNum = std::to_string(ViewportIndex);
    Config["CameraSpeedSetting"+ ViewportNum] = std::to_string(CameraSpeedSetting);
    Config["CameraSpeedScalar"+ ViewportNum] = std::to_string(CameraSpeed);
    Config["GridSize"+ ViewportNum] = std::to_string(GridSize);
    Config["PerspectiveCameraLocX" + ViewportNum] = std::to_string(PerspectiveCamera.GetLocation().X);
    Config["PerspectiveCameraLocY" + ViewportNum] = std::to_string(PerspectiveCamera.GetLocation().Y);
    Config["PerspectiveCameraLocZ" + ViewportNum] = std::to_string(PerspectiveCamera.GetLocation().Z);
    Config["PerspectiveCameraRotX" + ViewportNum] = std::to_string(PerspectiveCamera.GetRotation().X);
    Config["PerspectiveCameraRotY" + ViewportNum] = std::to_string(PerspectiveCamera.GetRotation().Y);
    Config["PerspectiveCameraRotZ" + ViewportNum] = std::to_string(PerspectiveCamera.GetRotation().Z);
    Config["ShowFlag"+ ViewportNum] = std::to_string(ShowFlag);
    Config["ViewMode" + ViewportNum] = std::to_string(static_cast<int32>(ViewMode));
    Config["ViewportType" + ViewportNum] = std::to_string(ViewportType);
}

TMap<FString, FString> FEditorViewportClient::ReadIniFile(const FString& FilePath)
{
    TMap<FString, FString> Config;
    std::ifstream File(*FilePath);
    std::string Line;

    while (std::getline(File, Line))
    {
        if (Line.empty() || Line[0] == '[' || Line[0] == ';')
        {
            continue;
        }
        std::istringstream SS(Line);
        std::string Key, Value;
        if (std::getline(SS, Key, '=') && std::getline(SS, Value))
        {
            Config[Key] = Value;
        }
    }
    return Config;
}

auto FEditorViewportClient::WriteIniFile(const FString& FilePath, const TMap<FString, FString>& Config) -> void
{
    std::ofstream File(*FilePath);
    for (const auto& Pair : Config)
    {
        File << *Pair.Key << "=" << *Pair.Value << "\n";
    }
}

void FEditorViewportClient::SetCameraSpeed(const float InValue)
{
    CameraSpeed = FMath::Clamp(InValue, 0.1f, 2000.f);
}

FVector FViewportCamera::GetForwardVector() const
{
    FVector Forward = FVector(1.f, 0.f, 0.0f);
    Forward = JungleMath::FVectorRotate(Forward, ViewRotation);
    return Forward;
}

FVector FViewportCamera::GetRightVector() const
{
    FVector Right = FVector(0.f, 1.f, 0.0f);
    Right = JungleMath::FVectorRotate(Right, ViewRotation);
    return Right;
}

FVector FViewportCamera::GetUpVector() const
{
    FVector Up = FVector(0.f, 0.f, 1.0f);
    Up = JungleMath::FVectorRotate(Up, ViewRotation);
    return Up;
}
