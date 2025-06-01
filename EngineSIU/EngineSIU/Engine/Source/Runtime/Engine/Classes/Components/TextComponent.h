#pragma once

#define _TCHAR_DEFINED
#include <wrl.h>

#include "BillboardComponent.h"

class UTextComponent : public UBillboardComponent
{
    DECLARE_CLASS(UTextComponent, UBillboardComponent)

public:
    UTextComponent();

    virtual UObject* Duplicate(UObject* InOuter) override;
    
    void GetProperties(TMap<FString, FString>& OutProperties) const override;
    
    void SetProperties(const TMap<FString, FString>& InProperties) override;

    virtual void InitializeComponent() override;
    
    virtual void TickComponent(float DeltaTime) override;
    
    void ClearText();
    
    void SetText(const FWString& InText);

    FWString GetText() { return Text; }
    
    void SetRowColumnCount(int CellsPerRow, int CellsPerColumn);

    virtual int CheckRayIntersection(const FVector& InRayOrigin, const FVector& InRayDirection, float& OutHitDistance) const override;
 
    
    int GetRowCount() const { return RowCount; }
    int GetColumnCount() const { return ColumnCount; }

protected:

    // TODO: 씬저장에 FString로 저장되는 문제 있음
    FWString Text;

    //TArray<FVertexTexture> vertexTextureArr;

    int QuadSize = 2;

    int RowCount;
    int ColumnCount;

    float QuadWidth = 2.0f;
    float QuadHeight = 2.0f;

private:
    //FString TextAtlasBufferKey;
};
