#include "BillboardComponent.h"
#include "Define.h"
#include "LevelEditor/SLevelEditor.h"
#include "Math/MathUtility.h"
#include "UnrealEd/EditorViewportClient.h"
#include "EngineLoop.h"
#include "UnrealClient.h"
#include "CoreUObject/UObject/Casts.h"

UBillboardComponent::UBillboardComponent()
{
    SetType(StaticClass()->GetName());
    SetTexture(L"Assets/Editor/Icon/S_Actor.PNG");
}

UObject* UBillboardComponent::Duplicate(UObject* InOuter)
{
    // GPU 버퍼는 공유하지 않고, 상태 값만 복사하여 새로 초기화하도록 함
    UBillboardComponent* NewComponent = Cast<UBillboardComponent>(Super::Duplicate(InOuter));
    if (NewComponent)
    {
        NewComponent->FinalIndexU = FinalIndexU;
        NewComponent->FinalIndexV = FinalIndexV;
        NewComponent->Texture = FEngineLoop::ResourceManager.GetTexture(TexturePath.ToWideString());
        NewComponent->TexturePath = TexturePath;
        NewComponent->UUIDParent = UUIDParent;
        NewComponent->bIsEditorBillboard = bIsEditorBillboard;
    }
    return NewComponent;
}

void UBillboardComponent::GetProperties(TMap<FString, FString>& OutProperties) const
{
    Super::GetProperties(OutProperties);
    OutProperties.Add(TEXT("FinalIndexU"), FString::Printf(TEXT("%f"), FinalIndexU));
    OutProperties.Add(TEXT("FinalIndexV"), FString::Printf(TEXT("%f"), FinalIndexV));
    OutProperties.Add(TEXT("BufferKey"), TexturePath);
}

void UBillboardComponent::SetProperties(const TMap<FString, FString>& InProperties)
{
    Super::SetProperties(InProperties);
    const FString* TempStr = nullptr;
    TempStr = InProperties.Find(TEXT("FinalIndexU"));
    if (TempStr)
    {
        FinalIndexU = FString::ToFloat(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("FinalIndexV"));
    if (TempStr)
    {
        FinalIndexV = FString::ToFloat(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("BufferKey"));
    if (TempStr)
    {
        TexturePath = *TempStr;
        Texture = FEngineLoop::ResourceManager.GetTexture(TempStr->ToWideString());
    }
}

void UBillboardComponent::InitializeComponent()
{
    Super::InitializeComponent();

}

void UBillboardComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);
}

int UBillboardComponent::CheckRayIntersection(const FVector& InRayOrigin, const FVector& InRayDirection, float& OutHitDistance) const
{
    TArray<FVector> Vertices =
    {
        FVector(-1.0f,  1.0f, 0.0f),
        FVector(1.0f,  1.0f, 0.0f),
        FVector(1.0f, -1.0f, 0.0f),
        FVector(-1.0f, -1.0f, 0.0f),
    };

    return CheckPickingOnNDC(Vertices, OutHitDistance) ? 1 : 0;
}

void UBillboardComponent::SetTexture(const FWString& InFilePath)
{
    Texture = FEngineLoop::ResourceManager.GetTexture(InFilePath);
    TexturePath = FString(InFilePath.c_str());
    //std::string str(_fileName.begin(), _fileName.end());
}

void UBillboardComponent::SetUUIDParent(USceneComponent* InUUIDParent)
{
    UUIDParent = InUUIDParent;
}

FMatrix UBillboardComponent::CreateBillboardMatrix() const
{
    // 카메라 뷰 행렬을 가져와서 위치 정보를 제거한 후 전치하여 LookAt 행렬 생성
    FMatrix CameraView = GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->GetViewMatrix();
    CameraView.M[0][3] = CameraView.M[1][3] = CameraView.M[2][3] = 0.0f;
    CameraView.M[3][0] = CameraView.M[3][1] = CameraView.M[3][2] = 0.0f;
    CameraView.M[3][3] = 1.0f;
    CameraView.M[0][2] = -CameraView.M[0][2];
    CameraView.M[1][2] = -CameraView.M[1][2];
    CameraView.M[2][2] = -CameraView.M[2][2];
    FMatrix LookAtCamera = FMatrix::Transpose(CameraView);

    FVector WorldLocation = GetComponentLocation();
    if (UUIDParent)
    {
        WorldLocation = UUIDParent->GetComponentLocation() + RelativeLocation;
    }
    
    FVector WorldScale = RelativeScale3D;
    FMatrix S = FMatrix::CreateScaleMatrix(WorldScale);
    FMatrix T = FMatrix::CreateTranslationMatrix(WorldLocation);
    
    // 최종 빌보드 행렬 = Scale * Rotation(LookAt) * Translation
    return S * LookAtCamera * T;
}


bool UBillboardComponent::CheckPickingOnNDC(const TArray<FVector>& QuadVertices, float& HitDistance) const
{
    // 마우스 위치를 클라이언트 좌표로 가져온 후 NDC 좌표로 변환
    POINT MousePos;
    GetCursorPos(&MousePos);
    ScreenToClient(GEngineLoop.AppWnd, &MousePos);

    const SLevelEditor* LevelEditor = GEngineLoop.GetLevelEditor();
    const std::shared_ptr<FEditorViewportClient> ActiveViewport = LevelEditor->GetActiveViewportClient();
    
    // NDC 좌표 계산: X, Y는 [-1,1] 범위로 매핑
    const FRect Rect = LevelEditor->GetActiveViewportClient()->GetViewport()->GetRect();
    const float MousePosX = static_cast<float>(MousePos.x) - Rect.TopLeftX;
    const float MousePosY = static_cast<float>(MousePos.y) - Rect.TopLeftY;
    const float NdcX = (2.0f * MousePosX / Rect.Width) - 1.0f;
    const float NdcY = -((2.0f * MousePosY / Rect.Height) - 1.0f);

    // MVP 행렬 계산
    const FMatrix M = CreateBillboardMatrix();
    const FMatrix V = ActiveViewport->GetViewMatrix();
    const FMatrix P = ActiveViewport->GetProjectionMatrix();
    const FMatrix MVP = M * V * P;

    // quadVertices를 MVP로 변환하여 NDC 공간에서의 최소/최대값 구하기
    float MinX = FLT_MAX, MaxX = -FLT_MAX;
    float MinY = FLT_MAX, MaxY = -FLT_MAX;

    for (const FVector& Position : QuadVertices)
    {
        FVector4 ClipPos = FMatrix::TransformVector(FVector4(Position, 1.0f), MVP);
        if (ClipPos.W != 0.0f)
        {
            ClipPos = ClipPos / ClipPos.W;
        }
        MinX = FMath::Min(MinX, ClipPos.X);
        MaxX = FMath::Max(MaxX, ClipPos.X);
        MinY = FMath::Min(MinY, ClipPos.Y);
        MaxY = FMath::Max(MaxY, ClipPos.Y);
    }

    // 마우스 NDC 좌표가 quad의 NDC 경계 사각형 내에 있는지 검사
    if (NdcX >= MinX && NdcX <= MaxX && NdcY >= MinY && NdcY <= MaxY)
    {
        const FVector WorldLocation = GetComponentLocation();
        const FVector CameraLocation = ActiveViewport->GetCameraLocation();
        HitDistance = (WorldLocation - CameraLocation).Length();
        return true;
    }
    return false;
}
