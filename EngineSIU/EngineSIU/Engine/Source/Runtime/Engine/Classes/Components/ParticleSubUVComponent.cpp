#include "ParticleSubUVComponent.h"
#include "EngineLoop.h"
#include "UObject/Casts.h"
#include "D3D11RHI/DXDBufferManager.h"

// 생성자: 기본 값 설정
UParticleSubUVComponent::UParticleSubUVComponent()
{
    SetType(StaticClass()->GetName());
    bIsLoop = true;
}

// Duplicate: 버퍼 포인터는 복사하지 않고 애니메이션 상태만 복제
UObject* UParticleSubUVComponent::Duplicate(UObject* InOuter)
{
    UParticleSubUVComponent* NewComponent = Cast<UParticleSubUVComponent>(Super::Duplicate(InOuter));
    if (NewComponent)
    {
        NewComponent->bIsLoop = bIsLoop;
        NewComponent->CellsPerRow = CellsPerRow;
        NewComponent->CellsPerColumn = CellsPerColumn;
        NewComponent->IndexU = IndexU;
        NewComponent->IndexV = IndexV;
        NewComponent->ElapsedTime = ElapsedTime;
        NewComponent->FrameDuration = FrameDuration;
        NewComponent->UVScale = UVScale;
        NewComponent->UVOffset = UVOffset;
    }
    return NewComponent;
}

void UParticleSubUVComponent::GetProperties(TMap<FString, FString>& OutProperties) const
{
    Super::GetProperties(OutProperties);
    OutProperties.Add(TEXT("bIsLoop"), bIsLoop ? TEXT("true") : TEXT("false"));
    OutProperties.Add(TEXT("CellsPerRow"), FString::Printf(TEXT("%d"), CellsPerRow));
    OutProperties.Add(TEXT("CellsPerColumn"), FString::Printf(TEXT("%d"), CellsPerColumn));
    OutProperties.Add(TEXT("IndexU"), FString::Printf(TEXT("%d"), IndexU));
    OutProperties.Add(TEXT("IndexV"), FString::Printf(TEXT("%d"), IndexV));
    OutProperties.Add(TEXT("ElapsedTime"), FString::Printf(TEXT("%f"), ElapsedTime));
    OutProperties.Add(TEXT("FrameDuration"), FString::Printf(TEXT("%f"), FrameDuration));
    OutProperties.Add(TEXT("UVScale"), FString::Printf(TEXT("%s"), *UVScale.ToString()));
    OutProperties.Add(TEXT("UVOffset"), FString::Printf(TEXT("%s"), *UVOffset.ToString()));
    
}

void UParticleSubUVComponent::SetProperties(const TMap<FString, FString>& InProperties)
{
    Super::SetProperties(InProperties);
    const FString* TempStr = nullptr;
    TempStr = InProperties.Find(TEXT("bIsLoop"));
    if (TempStr)
    {
        bIsLoop = (*TempStr == TEXT("true"));
    }
    TempStr = InProperties.Find(TEXT("CellsPerRow"));
    if (TempStr)
    {
        CellsPerRow = FString::ToInt(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("CellsPerColumn"));
    if (TempStr)
    {
        CellsPerColumn = FString::ToInt(**TempStr);
    }
    TempStr = InProperties.Find(TEXT("IndexU"));
    if (TempStr)
    {
        IndexU = FString::ToInt(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("IndexV"));
    if (TempStr)
    {
        IndexV = FString::ToInt(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("ElapsedTime"));
    if (TempStr)
    {
        ElapsedTime = FString::ToFloat(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("FrameDuration"));
    if (TempStr)
    {
        FrameDuration = FString::ToFloat(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("UVScale"));
    
    if (TempStr)
    {
        UVScale.InitFromString(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("UVOffset"));
    if (TempStr)
    {
        UVOffset.InitFromString(*TempStr);
    }
}

// InitializeComponent: 초기화 시 버텍스 버퍼 생성
void UParticleSubUVComponent::InitializeComponent()
{

}

// TickComponent: 매 프레임 애니메이션 업데이트 및 UV 오프셋 계산
void UParticleSubUVComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);
    if (!IsActive() || !Texture)
        return;

    // 텍스처 크기를 기반으로 셀 크기 계산 (행, 열)
    uint32 CellWidth = Texture->Width / CellsPerColumn;
    uint32 CellHeight = Texture->Height / CellsPerRow;

    float UVSaleX = 1.0f / static_cast<float>(CellsPerColumn);
    float UVSaleY = 1.0f / static_cast<float>(CellsPerRow);

    // 시간 누적 후 프레임 전환
    ElapsedTime += (DeltaTime * 1000);
    if (ElapsedTime >= FrameDuration)
    {
        IndexU++;
        ElapsedTime = 0.0f;
    }
    if (IndexU >= CellsPerColumn)
    {
        IndexU = 0;
        IndexV++;
    }
    if (IndexV >= CellsPerRow)
    {
        IndexU = 0;
        IndexV = 0;
        if (!bIsLoop)
        {
            Deactivate();
        }
    }

    float UVOffsetX = static_cast<float>(IndexU) * UVSaleX;
    float UVOffsetY = static_cast<float>(IndexV) * UVSaleY;

    UVScale = FVector2D(UVSaleX, UVSaleY);
    UVOffset = FVector2D(UVOffsetX, UVOffsetY);
}

// SetRowColumnCount: 셀 수 변경 시 버텍스 버퍼 재생성
void UParticleSubUVComponent::SetRowColumnCount(int InCellsPerRow, int InCellsPerColumn)
{
    CellsPerRow = InCellsPerRow;
    CellsPerColumn = InCellsPerColumn;
}

void UParticleSubUVComponent::SetTexture(const FWString& FileName)
{
    Texture = FEngineLoop::ResourceManager.GetTexture(FileName);
    std::string Str(FileName.begin(), FileName.end());

}
