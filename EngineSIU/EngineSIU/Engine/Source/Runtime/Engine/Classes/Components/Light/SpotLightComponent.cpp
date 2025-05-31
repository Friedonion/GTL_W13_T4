#include "SpotLightComponent.h"

#include "Math/JungleMath.h"
#include "Math/Rotator.h"
#include "Math/Quat.h"
#include "UObject/Casts.h"



USpotLightComponent::USpotLightComponent()
{
    SpotLightInfo.Position = GetComponentLocation();
    SpotLightInfo.Radius = 30.0f;
    SpotLightInfo.Direction = GetForwardVector();
    SpotLightInfo.LightColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
    SpotLightInfo.Intensity = 1000.0f;
    SpotLightInfo.Type = ELightType::SPOT_LIGHT;
    SpotLightInfo.InnerRad = 0.2618;
    SpotLightInfo.OuterRad = 0.5236;
    SpotLightInfo.Attenuation = 20.0f;
}

UObject* USpotLightComponent::Duplicate(UObject* InOuter)
{
    ThisClass* NewComponent = Cast<ThisClass>(Super::Duplicate(InOuter));
    if (NewComponent)
    {
        NewComponent->SpotLightInfo = SpotLightInfo;
    }
    
    return NewComponent;
}

void USpotLightComponent::GetProperties(TMap<FString, FString>& OutProperties) const
{
    Super::GetProperties(OutProperties);
    OutProperties.Add(TEXT("Position"), FString::Printf(TEXT("%s"), *SpotLightInfo.Position.ToString()));
    OutProperties.Add(TEXT("Radius"), FString::Printf(TEXT("%f"), SpotLightInfo.Radius));
    OutProperties.Add(TEXT("Direction"), FString::Printf(TEXT("%s"), *SpotLightInfo.Direction.ToString()));
    OutProperties.Add(TEXT("LightColor"), FString::Printf(TEXT("%s"), *SpotLightInfo.LightColor.ToString()));
    OutProperties.Add(TEXT("Intensity"), FString::Printf(TEXT("%f"), SpotLightInfo.Intensity));
    OutProperties.Add(TEXT("Type"), FString::Printf(TEXT("%d"), SpotLightInfo.Type));
    OutProperties.Add(TEXT("InnerRad"), FString::Printf(TEXT("%f"), SpotLightInfo.InnerRad));
    OutProperties.Add(TEXT("OuterRad"), FString::Printf(TEXT("%f"), SpotLightInfo.OuterRad));
    OutProperties.Add(TEXT("Attenuation"), FString::Printf(TEXT("%f"), SpotLightInfo.Attenuation));
    
}

void USpotLightComponent::SetProperties(const TMap<FString, FString>& InProperties)
{
    Super::SetProperties(InProperties);
    const FString* TempStr = nullptr;
    TempStr = InProperties.Find(TEXT("Position"));
    if (TempStr)
    {
        SpotLightInfo.Position.InitFromString(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("Radius"));
    if (TempStr)
    {
        SpotLightInfo.Radius = FString::ToFloat(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("Direction"));
    if (TempStr)
    {
        SpotLightInfo.Direction.InitFromString(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("LightColor"));
    if (TempStr)
    {
        SpotLightInfo.LightColor.InitFromString(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("Intensity"));
    if (TempStr)
    {
        SpotLightInfo.Intensity = FString::ToFloat(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("Type"));
    if (TempStr)
    {
        SpotLightInfo.Type = FString::ToInt(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("InnerRad"));
    if (TempStr)
    {
        SpotLightInfo.InnerRad = FString::ToFloat(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("OuterRad"));
    if (TempStr)
    {
        SpotLightInfo.OuterRad = FString::ToFloat(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("Attenuation"));
    if (TempStr)
    {
        SpotLightInfo.Attenuation = FString::ToFloat(*TempStr);
    }
}

FVector USpotLightComponent::GetDirection()
{
    return GetForwardVector();
}

FSpotLightInfo& USpotLightComponent::GetSpotLightInfo() 
{
    return SpotLightInfo;
}

void USpotLightComponent::SetSpotLightInfo(const FSpotLightInfo& InSpotLightInfo)
{
    SpotLightInfo = InSpotLightInfo;
}

float USpotLightComponent::GetRadius() const
{
    return SpotLightInfo.Radius;
}

void USpotLightComponent::SetRadius(float InRadius)
{
    SpotLightInfo.Radius = InRadius;
}

FLinearColor USpotLightComponent::GetLightColor() const
{
    return SpotLightInfo.LightColor;
}

void USpotLightComponent::SetLightColor(const FLinearColor& InColor)
{
    SpotLightInfo.LightColor = InColor;
}



float USpotLightComponent::GetIntensity() const
{
    return SpotLightInfo.Intensity;
}

void USpotLightComponent::SetIntensity(float InIntensity)
{
    SpotLightInfo.Intensity = InIntensity;
}

int USpotLightComponent::GetType() const
{
    return SpotLightInfo.Type;
}

void USpotLightComponent::SetType(int InType)
{
    SpotLightInfo.Type = InType;
}

float USpotLightComponent::GetInnerRad() const
{
    return SpotLightInfo.InnerRad;
}

void USpotLightComponent::SetInnerRad(float InInnerCos)
{
    SpotLightInfo.InnerRad = InInnerCos;
}

float USpotLightComponent::GetOuterRad() const
{
    return SpotLightInfo.OuterRad;
}

void USpotLightComponent::SetOuterRad(float InOuterCos)
{
    SpotLightInfo.OuterRad = InOuterCos;
}

float USpotLightComponent::GetInnerDegree() const
{
    return SpotLightInfo.InnerRad * (180.0f / PI);
}

void USpotLightComponent::SetInnerDegree(float InInnerDegree)
{
    SpotLightInfo.InnerRad = InInnerDegree * (PI / 180.0f);
}   

float USpotLightComponent::GetOuterDegree() const
{
    return SpotLightInfo.OuterRad * (180 / PI);
}

void USpotLightComponent::SetOuterDegree(float InOuterDegree)
{
    SpotLightInfo.OuterRad = InOuterDegree * (PI / 180.0f);
}

void USpotLightComponent::UpdateViewMatrix()
{
    FVector SpotLightPos = GetComponentLocation();
    FRotator SpotLightRot = GetComponentRotation(); // 월드 회전 값 가져오기

    // 월드 공간에서의 Forward 벡터 계산
    FVector Forward = FVector(1.f, 0.f, 0.0f); // 로컬 +X 축
    Forward = JungleMath::FVectorRotate(Forward, SpotLightRot);

    // 월드 공간에서의 Up 벡터 계산
    FVector Up = FVector(0.f, 0.f, 1.f); // 로컬 +Z 축 (또는 엔진 컨벤션에 맞는 로컬 Up)
    Up = JungleMath::FVectorRotate(Up, SpotLightRot);

    // 개선된 Up 벡터를 사용하여 View Matrix 생성
    ViewMatrices[0] = JungleMath::CreateViewMatrix(SpotLightPos, SpotLightPos + Forward, Up);
}

void USpotLightComponent::UpdateProjectionMatrix()
{
    // TODO: 텍스쳐 비율 반영
    AspectRatio = 1.0f;

    const float OuterHalfDegree = FMath::RadiansToDegrees(SpotLightInfo.OuterRad);
    const float OuterCosine = FMath::DegreesToRadians(OuterHalfDegree * 2.f);
    // 각도(radian) = acos(cos(angle)) * 2. 스포트라이트 FOV는 전체 원뿔 각도임.
    const float FieldOfViewRadians = OuterCosine;

    // Near Clip Plane 값 설정 (매우 작은 값 사용)
    const float NearClipPlane = NEAR_PLANE; // 또는 직접 상수 값 사용 (예: 0.01f)

    // Far Clip Plane 값 설정 (라이트의 감쇠 반경 사용)
    const float FarClipPlane = SpotLightInfo.Radius; // 또는 GetRadius() 함수 사용 가능


    ProjectionMatrix = JungleMath::CreateProjectionMatrix(
        FieldOfViewRadians,
        AspectRatio,
        NearClipPlane,
        FarClipPlane
    );
    
}
