#include "Player.h"

#include "UnrealClient.h"
#include "World/World.h"
#include "BaseGizmos/GizmoArrowComponent.h"
#include "BaseGizmos/GizmoCircleComponent.h"
#include "BaseGizmos/TransformGizmo.h"
#include "Components/Light/LightComponent.h"
#include "LevelEditor/SLevelEditor.h"
#include "Math/JungleMath.h"
#include "Math/MathUtility.h"
#include "PropertyEditor/ShowFlags.h"
#include "UnrealEd/EditorViewportClient.h"
#include "UObject/UObjectIterator.h"
#include "Engine/EditorEngine.h"
#include "Engine/SkeletalMesh.h"
#include "Lua/LuaScriptComponent.h"
#include "Lua/LuaScriptManager.h"
#include "Lua/LuaUtils/LuaTypeMacros.h"

void AEditorPlayer::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    Input();
}

void AEditorPlayer::Input()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;
    if (io.WantCaptureKeyboard) return;

    if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
    {
        if (!bLeftMouseDown)
        {   
            bLeftMouseDown = true;

            POINT mousePos;
            GetCursorPos(&mousePos);
            GetCursorPos(&LastMousePos);
            ScreenToClient(GEngineLoop.AppWnd, &mousePos);

            /*
            uint32 UUID = FEngineLoop::GraphicDevice.GetPixelUUID(mousePos);
            // TArray<UObject*> objectArr = GetWorld()->GetObjectArr();
            for ( const USceneComponent* obj : TObjectRange<USceneComponent>())
            {
                if (obj->GetUUID() != UUID) continue;

                UE_LOG(ELogLevel::Display, *obj->GetName());
            }
            */

            FVector pickPosition;

            std::shared_ptr<FEditorViewportClient> ActiveViewport = GEngineLoop.GetLevelEditor()->GetActiveViewportClient();
            ScreenToViewSpace(mousePos.x, mousePos.y, ActiveViewport, pickPosition);
            bool res = PickGizmo(pickPosition, ActiveViewport.get());
            if (!res) PickActor(pickPosition);
            if (res)
            {
                UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);
                if (Engine->ActiveWorld->WorldType == EWorldType::SkeletalViewer)
                if (USkeletalMeshComponent* SkeletalMeshComp = Cast<USkeletalMeshComponent>(Engine->GetSelectedComponent()))
                {
                    UGizmoBaseComponent* Gizmo = Cast<UGizmoBaseComponent>(ActiveViewport->GetPickedGizmoComponent());
                    int BoneIndex = Engine->SkeletalMeshViewerWorld->SelectBoneIndex;
                    TArray<FMatrix> GlobalBoneMatrices;
                    SkeletalMeshComp->GetCurrentGlobalBoneMatrices(GlobalBoneMatrices);

                    FTransform GlobalBoneTransform = FTransform(GlobalBoneMatrices[BoneIndex]);
                    InitialBoneRotationForGizmo = GlobalBoneTransform.GetRotation();
                }
                //bIsGizmoDragging = true;
                //GizmoDrag_InitialLocalXAxis = InitialBoneRotationForGizmo.RotateVector(FVector::ForwardVector); // 또는 (1,0,0) 등 FBX 기준 축
                //GizmoDrag_InitialLocalYAxis = InitialBoneRotationForGizmo.RotateVector(FVector::RightVector);
                //GizmoDrag_InitialLocalZAxis = InitialBoneRotationForGizmo.RotateVector(FVector::UpVector);
            }
            
        }
        else
        {
            UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);
            if (Engine->ActiveWorld->WorldType == EWorldType::Editor)
            {
                PickedObjControl();
            }
            else if (Engine->ActiveWorld->WorldType == EWorldType::SkeletalViewer)
            {
                PickedBoneControl();
            }
        }
    }
    else
    {
        if (bLeftMouseDown)
        {
            bLeftMouseDown = false;
            std::shared_ptr<FEditorViewportClient> ActiveViewport = GEngineLoop.GetLevelEditor()->GetActiveViewportClient();
            ActiveViewport->SetPickedGizmoComponent(nullptr);
        }
    }
}

void AEditorPlayer::ProcessGizmoIntersection(UStaticMeshComponent* Component, const FVector& PickPosition, FEditorViewportClient* InActiveViewport, bool& bIsPickedGizmo)
{
    int maxIntersect = 0;
    float minDistance = FLT_MAX;
    float Distance = 0.0f;
    int currentIntersectCount = 0;
    if (!Component) return;
    if (RayIntersectsObject(PickPosition, Component, Distance, currentIntersectCount))
    {
        if (Distance < minDistance)
        {
            minDistance = Distance;
            maxIntersect = currentIntersectCount;
            //GetWorld()->SetPickingGizmo(iter);
            InActiveViewport->SetPickedGizmoComponent(Component);
            bIsPickedGizmo = true;
        }
        else if (abs(Distance - minDistance) < FLT_EPSILON && currentIntersectCount > maxIntersect)
        {
            maxIntersect = currentIntersectCount;
            //GetWorld()->SetPickingGizmo(iter);
            InActiveViewport->SetPickedGizmoComponent(Component);
            bIsPickedGizmo = true;
        }
    }
}

bool AEditorPlayer::PickGizmo(FVector& pickPosition, FEditorViewportClient* InActiveViewport)
{
    bool isPickedGizmo = false;
    UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);
    if (Engine->GetSelectedActor())
    {
        if (ControlMode == CM_TRANSLATION)
        {
            for (UStaticMeshComponent* iter : InActiveViewport->GetGizmoActor()->GetArrowArr())
            {
                ProcessGizmoIntersection(iter, pickPosition, InActiveViewport, isPickedGizmo);
            }
        }
        else if (ControlMode == CM_ROTATION)
        {
            for (UStaticMeshComponent* iter : InActiveViewport->GetGizmoActor()->GetDiscArr())
            {
                ProcessGizmoIntersection(iter, pickPosition, InActiveViewport, isPickedGizmo);
            }
        }
        else if (ControlMode == CM_SCALE)
        {
            for (UStaticMeshComponent* iter : InActiveViewport->GetGizmoActor()->GetScaleArr())
            {
                ProcessGizmoIntersection(iter, pickPosition, InActiveViewport, isPickedGizmo);
            }
        }
    }
    return isPickedGizmo;
}

void AEditorPlayer::PickActor(const FVector& PickPosition)
{
    if (!(ShowFlags::GetInstance().CurrentFlags & EEngineShowFlags::SF_Primitives)) return;

    USceneComponent* Possible = nullptr;
    int maxIntersect = 0;
    float minDistance = FLT_MAX;
    for (const auto iter : TObjectRange<UPrimitiveComponent>())
    {
        UPrimitiveComponent* pObj;
        if (iter->IsA<UPrimitiveComponent>() || iter->IsA<ULightComponentBase>())
        {
            pObj = static_cast<UPrimitiveComponent*>(iter);
        }
        else
        {
            continue;
        }

        if (pObj && !pObj->IsA<UGizmoBaseComponent>())
        {
            float Distance = 0.0f;
            int currentIntersectCount = 0;
            if (RayIntersectsObject(PickPosition, pObj, Distance, currentIntersectCount))
            {
                if (Distance < minDistance)
                {
                    minDistance = Distance;
                    maxIntersect = currentIntersectCount;
                    Possible = pObj;
                }
                else if (abs(Distance - minDistance) < FLT_EPSILON && currentIntersectCount > maxIntersect)
                {
                    maxIntersect = currentIntersectCount;
                    Possible = pObj;
                }
            }
        }
    }
    if (Possible)
    {
        Cast<UEditorEngine>(GEngine)->SelectActor(Possible->GetOwner());
        Cast<UEditorEngine>(GEngine)->SelectComponent(Possible);
    }
    else
    {
        Cast<UEditorEngine>(GEngine)->DeselectActor(Cast<UEditorEngine>(GEngine)->GetSelectedActor());
        Cast<UEditorEngine>(GEngine)->DeselectComponent(Cast<UEditorEngine>(GEngine)->GetSelectedComponent());
    }
}

void AEditorPlayer::AddControlMode()
{
    ControlMode = static_cast<EControlMode>((ControlMode + 1) % CM_END);
}

void AEditorPlayer::AddCoordMode()
{
    CoordMode = static_cast<ECoordMode>((CoordMode + 1) % CDM_END);
}

void AEditorPlayer::ScreenToViewSpace(int32 ScreenX, int32 ScreenY, std::shared_ptr<FEditorViewportClient> ActiveViewport, FVector& RayOrigin)
{
    FRect Rect = ActiveViewport->GetViewport()->GetRect();
    
    float ViewportX = static_cast<float>(ScreenX) - Rect.TopLeftX;
    float ViewportY = static_cast<float>(ScreenY) - Rect.TopLeftY;

    FMatrix ProjectionMatrix = ActiveViewport->GetProjectionMatrix();
    
    RayOrigin.X = ((2.0f * ViewportX / Rect.Width) - 1) / ProjectionMatrix[0][0];
    RayOrigin.Y = -((2.0f * ViewportY / Rect.Height) - 1) / ProjectionMatrix[1][1];
    
    if (GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->IsOrthographic())
    {
        RayOrigin.Z = 0.0f;  // 오쏘 모드에서는 unproject 시 near plane 위치를 기준
    }
    else
    {
        RayOrigin.Z = 1.0f;  // 퍼스펙티브 모드: near plane
    }
}

int AEditorPlayer::RayIntersectsObject(const FVector& PickPosition, USceneComponent* Component, float& HitDistance, int& IntersectCount)
{
    FMatrix WorldMatrix = Component->GetWorldMatrix();
    FMatrix ViewMatrix = GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->GetViewMatrix();
    
    bool bIsOrtho = GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->IsOrthographic();
    

    if (bIsOrtho)
    {
        // 오쏘 모드: ScreenToViewSpace()에서 계산된 pickPosition이 클립/뷰 좌표라고 가정
        FMatrix inverseView = FMatrix::Inverse(ViewMatrix);
        // pickPosition을 월드 좌표로 변환
        FVector worldPickPos = inverseView.TransformPosition(PickPosition);  
        // 오쏘에서는 픽킹 원점은 unproject된 픽셀의 위치
        FVector rayOrigin = worldPickPos;
        // 레이 방향은 카메라의 정면 방향 (평행)
        FVector orthoRayDir = GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->OrthogonalCamera.GetForwardVector().GetSafeNormal();

        // 객체의 로컬 좌표계로 변환
        FMatrix LocalMatrix = FMatrix::Inverse(WorldMatrix);
        FVector LocalRayOrigin = LocalMatrix.TransformPosition(rayOrigin);
        FVector LocalRayDir = (LocalMatrix.TransformPosition(rayOrigin + orthoRayDir) - LocalRayOrigin).GetSafeNormal();
        
        IntersectCount = Component->CheckRayIntersection(LocalRayOrigin, LocalRayDir, HitDistance);
        return IntersectCount;
    }
    else
    {
        FMatrix InverseMatrix = FMatrix::Inverse(WorldMatrix * ViewMatrix);
        FVector CameraOrigin = { 0,0,0 };
        FVector PickRayOrigin = InverseMatrix.TransformPosition(CameraOrigin);
        
        // 퍼스펙티브 모드의 기존 로직 사용
        FVector TransformedPick = InverseMatrix.TransformPosition(PickPosition);
        FVector RayDirection = (TransformedPick - PickRayOrigin).GetSafeNormal();
        
        IntersectCount = Component->CheckRayIntersection(PickRayOrigin, RayDirection, HitDistance);

        if (IntersectCount > 0)
        {
            FVector LocalHitPoint = PickRayOrigin + RayDirection * HitDistance;

            FVector WorldHitPoint = WorldMatrix.TransformPosition(LocalHitPoint);

            FMatrix InverseView = FMatrix::Inverse(ViewMatrix);
            FVector WorldRayOrigin = InverseView.TransformPosition(CameraOrigin);

            float WorldDistance = FVector::Distance(WorldRayOrigin, WorldHitPoint);

            HitDistance = WorldDistance;
        }
        return IntersectCount;
    }
}

void AEditorPlayer::PickedObjControl()
{
    UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);
    FEditorViewportClient* ActiveViewport = GEngineLoop.GetLevelEditor()->GetActiveViewportClient().get();
    if (Engine && Engine->GetSelectedActor() && ActiveViewport->GetPickedGizmoComponent())
    {
        POINT CurrentMousePos;
        GetCursorPos(&CurrentMousePos);
        const float DeltaX = static_cast<float>(CurrentMousePos.x - LastMousePos.x);
        const float DeltaY = static_cast<float>(CurrentMousePos.y - LastMousePos.y);

        USceneComponent* TargetComponent = Engine->GetSelectedComponent();
        if (!TargetComponent)
        {
            if (AActor* SelectedActor = Engine->GetSelectedActor())
            {
                TargetComponent = SelectedActor->GetRootComponent();
            }
            else
            {
                return;
            }
        }
        
        UGizmoBaseComponent* Gizmo = Cast<UGizmoBaseComponent>(ActiveViewport->GetPickedGizmoComponent());
        switch (ControlMode)
        {
        case CM_TRANSLATION:
            // ControlTranslation(TargetComponent, Gizmo, deltaX, deltaY);
            // SLevelEditor에 있음
            break;
        case CM_SCALE:
            ControlScale(TargetComponent, Gizmo, DeltaX, DeltaY);
            break;
        case CM_ROTATION:
            ControlRotation(TargetComponent, Gizmo, DeltaX, DeltaY);
            break;
        default:
            break;
        }
        LastMousePos = CurrentMousePos;
    }
}

void AEditorPlayer::PickedBoneControl()
{
    UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);
    FEditorViewportClient* ActiveViewport = GEngineLoop.GetLevelEditor()->GetActiveViewportClient().get();
    if (Engine && Engine->GetSelectedActor() && ActiveViewport->GetPickedGizmoComponent())
    {
        
        POINT CurrentMousePos;
        GetCursorPos(&CurrentMousePos);
        const float DeltaX = static_cast<float>(CurrentMousePos.x - LastMousePos.x);
        const float DeltaY = static_cast<float>(CurrentMousePos.y - LastMousePos.y);

        if (DeltaX > 1.f)
        {
            int a = 0;
        }

        USceneComponent* TargetComponent = Engine->GetSelectedComponent();
        if (!TargetComponent)
        {
            if (AActor* SelectedActor = Engine->GetSelectedActor())
            {
                TargetComponent = SelectedActor->GetRootComponent();
            }
            else
            {
                return;
            }
        }

        if (USkeletalMeshComponent* SkeletalMeshComp = Cast<USkeletalMeshComponent>(TargetComponent))
        {
            UGizmoBaseComponent* Gizmo = Cast<UGizmoBaseComponent>(ActiveViewport->GetPickedGizmoComponent());
            int BoneIndex = Engine->SkeletalMeshViewerWorld->SelectBoneIndex;
            TArray<FMatrix> GlobalBoneMatrices;
            SkeletalMeshComp->GetCurrentGlobalBoneMatrices(GlobalBoneMatrices);

            FTransform GlobalBoneTransform = FTransform(GlobalBoneMatrices[BoneIndex]);


            switch (ControlMode)
            {
            case CM_TRANSLATION:
                // ControlTranslation(TargetComponent, Gizmo, deltaX, deltaY);
                    // SLevelEditor에 있음
                        break;
            case CM_SCALE:
                {
                    FVector ScaleDelta = ControlBoneScale(GlobalBoneTransform, Gizmo, DeltaX, DeltaY);
                    SkeletalMeshComp->RefBonePoseTransforms[BoneIndex].Scale3D += ScaleDelta;
                }
                break;
            case CM_ROTATION:
                {
                    FQuat RotationDelta = ControlBoneRotation(GlobalBoneTransform, Gizmo, DeltaX, DeltaY);
                    SkeletalMeshComp->RefBonePoseTransforms[BoneIndex].Rotation = RotationDelta * SkeletalMeshComp->RefBonePoseTransforms[BoneIndex].Rotation;
                }
                break;
            default:
                break;
            }
            // 본의 로컬 변환을 업데이트
            //SkeletalMeshComp->BoneTransforms[BoneIndex] = GlobalBoneTransform;
        }
        
        LastMousePos = CurrentMousePos;
    }
}

void AEditorPlayer::ControlRotation(USceneComponent* Component, UGizmoBaseComponent* Gizmo, float DeltaX, float DeltaY)
{
    const auto ActiveViewport = GEngineLoop.GetLevelEditor()->GetActiveViewportClient();
    const FViewportCamera* ViewTransform = ActiveViewport->GetViewportType() == LVT_Perspective
                                                        ? &ActiveViewport->PerspectiveCamera
                                                        : &ActiveViewport->OrthogonalCamera;

    FVector CameraForward = ViewTransform->GetForwardVector();
    FVector CameraRight = ViewTransform->GetRightVector();
    FVector CameraUp = ViewTransform->GetUpVector();

    FQuat CurrentRotation = Component->GetComponentRotation().Quaternion();

    FQuat RotationDelta = FQuat();

    if (Gizmo->GetGizmoType() == UGizmoBaseComponent::CircleX)
    {
        float RotationAmount = (CameraUp.Z >= 0 ? -1.0f : 1.0f) * DeltaY * 0.01f;
        RotationAmount = RotationAmount + (CameraRight.X >= 0 ? 1.0f : -1.0f) * DeltaX * 0.01f;

        FVector Axis = FVector::ForwardVector;
        if (CoordMode == CDM_LOCAL)
        {
            Axis = Component->GetForwardVector();
        }

        RotationDelta = FQuat(Axis, RotationAmount);
    }
    else if (Gizmo->GetGizmoType() == UGizmoBaseComponent::CircleY)
    {
        float RotationAmount = (CameraRight.X >= 0 ? 1.0f : -1.0f) * DeltaX * 0.01f;
        RotationAmount = RotationAmount + (CameraUp.Z >= 0 ? 1.0f : -1.0f) * DeltaY * 0.01f;

        FVector Axis = FVector::RightVector;
        if (CoordMode == CDM_LOCAL)
        {
            Axis = Component->GetRightVector();
        }

        RotationDelta = FQuat(Axis, RotationAmount);
    }
    else if (Gizmo->GetGizmoType() == UGizmoBaseComponent::CircleZ)
    {
        float RotationAmount = (CameraForward.X <= 0 ? -1.0f : 1.0f) * DeltaX * 0.01f;

        FVector Axis = FVector::UpVector;
        if (CoordMode == CDM_LOCAL)
        {
            Axis = Component->GetUpVector();
        }
        
        RotationDelta = FQuat(Axis, RotationAmount);
    }

    // 쿼터니언의 곱 순서는 delta * current 가 맞음.
    Component->SetWorldRotation(RotationDelta * CurrentRotation); 
}

void AEditorPlayer::ControlScale(USceneComponent* Component, UGizmoBaseComponent* Gizmo, float DeltaX, float DeltaY)
{
    const auto ActiveViewport = GEngineLoop.GetLevelEditor()->GetActiveViewportClient();
    const FViewportCamera* ViewTransform = ActiveViewport->GetViewportType() == LVT_Perspective
                                                        ? &ActiveViewport->PerspectiveCamera
                                                        : &ActiveViewport->OrthogonalCamera;
    FVector CameraRight = ViewTransform->GetRightVector();
    FVector CameraUp = ViewTransform->GetUpVector();
    
    // 월드 좌표계에서 카메라 방향을 고려한 이동
    if (Gizmo->GetGizmoType() == UGizmoBaseComponent::ScaleX)
    {
        // 카메라의 오른쪽 방향을 X축 이동에 사용
        FVector moveDir = CameraRight * DeltaX * 0.05f;
        Component->AddScale(FVector(moveDir.X, 0.0f, 0.0f));
    }
    else if (Gizmo->GetGizmoType() == UGizmoBaseComponent::ScaleY)
    {
        // 카메라의 오른쪽 방향을 Y축 이동에 사용
        FVector moveDir = CameraRight * DeltaX * 0.05f;
        Component->AddScale(FVector(0.0f, moveDir.Y, 0.0f));
    }
    else if (Gizmo->GetGizmoType() == UGizmoBaseComponent::ScaleZ)
    {
        // 카메라의 위쪽 방향을 Z축 이동에 사용
        FVector moveDir = CameraUp * -DeltaY * 0.05f;
        Component->AddScale(FVector(0.0f, 0.0f, moveDir.Z));
    }
}
FQuat AEditorPlayer::ControlBoneRotation(FTransform& BoneTransform, UGizmoBaseComponent* Gizmo, float DeltaX, float DeltaY)
{
    const auto ActiveViewport = GEngineLoop.GetLevelEditor()->GetActiveViewportClient();
    const FViewportCamera* ViewTransform = ActiveViewport->GetViewportType() == LVT_Perspective
                                                        ? &ActiveViewport->PerspectiveCamera
                                                        : &ActiveViewport->OrthogonalCamera;

    FVector CameraForward = ViewTransform->GetForwardVector().GetSafeNormal(); // 정규화
    FVector CameraRight = ViewTransform->GetRightVector().GetSafeNormal();   // 정규화
    FVector CameraUp = ViewTransform->GetUpVector().GetSafeNormal();         // 정규화

    FQuat CurrentRotation = BoneTransform.GetRotation(); // 현재 회전은 여전히 필요 (결과 적용 시)
    FQuat RotationDelta = FQuat::Identity;
    float Sensitivity = 0.01f;

    FVector AxisToRotateAround = FVector::ZeroVector;

    if (Gizmo->GetGizmoType() == UGizmoBaseComponent::CircleX)
    {
        if (CoordMode == CDM_LOCAL)
        {
            // AxisToRotateAround = GizmoDrag_InitialLocalXAxis; // 드래그 시작 시 저장된 로컬 축 사용
             AxisToRotateAround = InitialBoneRotationForGizmo.RotateVector(FVector::ForwardVector); // 매번 초기 회전 기준으로 로컬축 계산
        }
        else // CDM_WORLD
        {
            AxisToRotateAround = FVector::ForwardVector; // 월드 X축
        }
    }
    else if (Gizmo->GetGizmoType() == UGizmoBaseComponent::CircleY)
    {
        if (CoordMode == CDM_LOCAL)
        {
            // AxisToRotateAround = GizmoDrag_InitialLocalYAxis;
            AxisToRotateAround = InitialBoneRotationForGizmo.RotateVector(FVector::RightVector);
        }
        else // CDM_WORLD
        {
            AxisToRotateAround = FVector::RightVector; // 월드 Y축
        }
    }
    else if (Gizmo->GetGizmoType() == UGizmoBaseComponent::CircleZ)
    {
        if (CoordMode == CDM_LOCAL)
        {
            // AxisToRotateAround = GizmoDrag_InitialLocalZAxis;
            AxisToRotateAround = InitialBoneRotationForGizmo.RotateVector(FVector::UpVector);
        }
        else // CDM_WORLD
        {
            AxisToRotateAround = FVector::UpVector; // 월드 Z축
        }
    }

    if (!AxisToRotateAround.IsNearlyZero()) // 유효한 축이 설정되었는지 확인
    {
        AxisToRotateAround.Normalize();
        float RotationAmount = 0.0f;

        // --- RotationAmount 계산 로직 (이 부분은 여전히 개선 필요) ---
        // 여기서는 매우 단순화된 예시: Gizmo 타입에 따라 DeltaX 또는 DeltaY 사용
        if (Gizmo->GetGizmoType() == UGizmoBaseComponent::CircleX) {
            RotationAmount = DeltaY * Sensitivity;
            // 화면 Y축 방향과 회전축(로컬X)의 관계에 따라 부호 조정 필요
            // 예: if (FVector::DotProduct(CameraUp, AxisToRotateAround) < 0) RotationAmount *= -1.0f;
        } else if (Gizmo->GetGizmoType() == UGizmoBaseComponent::CircleY) {
            RotationAmount = DeltaX * Sensitivity;
            // 화면 X축 방향과 회전축(로컬Y)의 관계에 따라 부호 조정 필요
            // 예: if (FVector::DotProduct(CameraRight, AxisToRotateAround) > 0) RotationAmount *= -1.0f;
        } else if (Gizmo->GetGizmoType() == UGizmoBaseComponent::CircleZ) {
            // Z축(Roll)은 보통 DeltaX (또는 화면 중심 기준 각도 변화)
            RotationAmount = DeltaX * Sensitivity;
            // 카메라가 축을 어떻게 보는지에 따라 부호 조정 필요
        }
        // --- RotationAmount 계산 로직 끝 ---

        RotationDelta = FQuat(AxisToRotateAround, RotationAmount);
    }

    // 반환된 RotationDelta는 호출부에서 다음과 같이 적용:
    // BoneTransform.SetRotation(RotationDelta * BoneTransform.GetRotation());
    // 또는 BoneTransform.ConcatenateRotation(RotationDelta); // UE에 이런 함수가 있다면
    return RotationDelta;
}

FVector AEditorPlayer::ControlBoneScale(FTransform& BoneTransform, UGizmoBaseComponent* Gizmo, float DeltaX, float DeltaY)
{
    const auto ActiveViewport = GEngineLoop.GetLevelEditor()->GetActiveViewportClient();
    const FViewportCamera* ViewTransform = ActiveViewport->GetViewportType() == LVT_Perspective
                                                        ? &ActiveViewport->PerspectiveCamera
                                                        : &ActiveViewport->OrthogonalCamera;
    FVector CameraRight = ViewTransform->GetRightVector();
    FVector CameraForward = ViewTransform->GetForwardVector();
    FVector CameraUp = ViewTransform->GetUpVector();
    FVector BoneScale;
    
    // 월드 좌표계에서 카메라 방향을 고려한 이동
    if (Gizmo->GetGizmoType() == UGizmoBaseComponent::ScaleX)
    {
        // 카메라의 오른쪽 방향을 X축 이동에 사용
        FVector moveDir = CameraForward * DeltaX * 0.05f;
        BoneScale = (FVector(moveDir.X, 0.0f, 0.0f));
    }
    else if (Gizmo->GetGizmoType() == UGizmoBaseComponent::ScaleY)
    {
        // 카메라의 오른쪽 방향을 Y축 이동에 사용
        FVector moveDir = CameraRight * DeltaX * 0.05f;
        BoneScale =  (FVector(0.0f, moveDir.Y, 0.0f));
    }
    else if (Gizmo->GetGizmoType() == UGizmoBaseComponent::ScaleZ)
    {
        // 카메라의 위쪽 방향을 Z축 이동에 사용
        FVector moveDir = CameraUp * -DeltaY * 0.05f;
        BoneScale = (FVector(0.0f, 0.0f, moveDir.Z));
    }
    
    return BoneScale;
}

void APlayer::BeginPlay()
{
    Super::BeginPlay();

    sol::state& Lua = FLuaScriptManager::Get().GetLua();

    // Lua -> C++로 연결(아직 호출은 안함)
    Lua.set_function("RegisterKeyCallback", 
        [](const std::string& Key, const std::function<void(float)>& Callback)
        {
            GEngine->ActiveWorld->GetPlayerController()->BindKey(FString(Key), Callback);
        }
    );

    Lua.set_function("RegisterMouseMoveCallback",
        [](const std::function<void(float, float)>& Callback)
        {
            GEngine->ActiveWorld->GetPlayerController()->BindMouseMove(Callback);
        }
    );
    // C++코드를 호출
    LuaScriptComponent->ActivateFunction("InitializeCallback");
}

UObject* APlayer::Duplicate(UObject* InOuter)
{
    ThisClass* NewActor = Cast<ThisClass>(Super::Duplicate(InOuter));

    return NewActor;
}

void APlayer::Tick(float DeltaTime)
{
    AActor::Tick(DeltaTime);
}

void APlayer::RegisterLuaType(sol::state& Lua)
{
    Super::RegisterLuaType(Lua);
}

ASequencerPlayer::ASequencerPlayer()
{
}

void ASequencerPlayer::PostSpawnInitialize()
{
    APlayer::PostSpawnInitialize();
    
    RootComponent = AddComponent<USceneComponent>();

    CameraComponent = AddComponent<UCameraComponent>();
    CameraComponent->SetupAttachment(RootComponent);
}

void ASequencerPlayer::Tick(float DeltaTime)
{
    APlayer::Tick(DeltaTime);

    if (SkeletalMeshComponent)
    {
        const FTransform SocketTransform = SkeletalMeshComponent->GetSocketTransform(Socket);
        SetActorRotation(SocketTransform.GetRotation().Rotator());
        SetActorLocation(SocketTransform.GetTranslation());
    }
}

UObject* ASequencerPlayer::Duplicate(UObject* InOuter)
{
    ThisClass* NewActor = Cast<ThisClass>(Super::Duplicate(InOuter));

    NewActor->Socket = Socket;
    NewActor->SkeletalMeshComponent = nullptr;

    return NewActor;
}
