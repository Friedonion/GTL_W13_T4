#include "StaticMesh.h"
#include "Engine/FObjLoader.h"
#include "UObject/Casts.h"
#include "UObject/ObjectFactory.h"

#include "Engine/Asset/StaticMeshAsset.h"

UObject* UStaticMesh::Duplicate(UObject* InOuter)
{
    // TODO: Context->CopyResource를 사용해서 Buffer복사
    // ThisClass* NewComponent = Cast<ThisClass>(Super::Duplicate());
    return nullptr;
}

int32 UStaticMesh::GetMaterialIndex(FName MaterialSlotName) const
{
    for (int32 MaterialIndex = 0; MaterialIndex < Materials.Num(); MaterialIndex++)
    {
        if (Materials[MaterialIndex]->MaterialSlotName == MaterialSlotName)
        {
            return MaterialIndex;
        }
    }

    return -1;
}

void UStaticMesh::GetUsedMaterials(TArray<UMaterial*>& OutMaterial) const
{
    for (const FStaticMaterial* Material : Materials)
    {
        OutMaterial.Emplace(Material->Material);
    }
}

FWString UStaticMesh::GetOjbectName() const
{
    return RenderData->ObjectName;
}

void UStaticMesh::SetData(FStaticMeshRenderData* InRenderData)
{
    RenderData = InRenderData;

    for (int MaterialIndex = 0; MaterialIndex < RenderData->Materials.Num(); MaterialIndex++)
    {
        FStaticMaterial* NewMaterialSlot = new FStaticMaterial();
        UMaterial* NewMaterial = FObjManager::CreateMaterial(RenderData->Materials[MaterialIndex]);

        NewMaterialSlot->Material = NewMaterial;
        NewMaterialSlot->MaterialSlotName = RenderData->Materials[MaterialIndex].MaterialName;

        Materials.Add(NewMaterialSlot);
    }
}

void UStaticMesh::SerializeAsset(FArchive& Ar)
{
    if (Ar.IsLoading())
    {
        if (!RenderData)
        {
            RenderData = new FStaticMeshRenderData();
        }
    }

    RenderData->Serialize(Ar);
}
