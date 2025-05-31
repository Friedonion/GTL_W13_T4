#pragma once
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "Components/Material/Material.h"
#include "Define.h"

class UBodySetup;
struct FStaticMeshRenderData;

class UStaticMesh : public UObject
{
    DECLARE_CLASS(UStaticMesh, UObject)

public:
    UStaticMesh() = default;

    virtual UObject* Duplicate(UObject* InOuter) override;

    const TArray<FStaticMaterial*>& GetMaterials() const { return Materials; }
    int32 GetMaterialIndex(FName MaterialSlotName) const;
    void GetUsedMaterials(TArray<UMaterial*>& OutMaterial) const;
    FStaticMeshRenderData* GetRenderData() const { return RenderData; }

    //ObjectName은 경로까지 포함
    FWString GetOjbectName() const;

    void SetData(FStaticMeshRenderData* InRenderData);

    virtual void SerializeAsset(FArchive& Ar) override;

private:
    FStaticMeshRenderData* RenderData = nullptr;
    TArray<FStaticMaterial*> Materials;
};
