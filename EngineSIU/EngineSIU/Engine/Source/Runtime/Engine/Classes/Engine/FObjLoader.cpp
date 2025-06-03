#include "FObjLoader.h"

#include "UObject/ObjectFactory.h"
#include "Components/Material/Material.h"
#include "Engine/StaticMesh.h"

#include "Asset/StaticMeshAsset.h"

#include <fstream>
#include <sstream>

bool FObjLoader::ParseObj(const FString& ObjFilePath, FObjInfo& OutObjInfo)
{
    std::ifstream Obj(ObjFilePath.ToWideString());
    if (!Obj)
    {
        return false;
    }

    OutObjInfo.FilePath = ObjFilePath.ToWideString().substr(0, ObjFilePath.ToWideString().find_last_of(L"\\/") + 1);
    OutObjInfo.ObjectName = ObjFilePath.ToWideString();
    // ObjectName은 wstring 타입이므로, 이를 string으로 변환 (간단한 ASCII 변환의 경우)
    std::wstring WideName = OutObjInfo.ObjectName.substr(ObjFilePath.ToWideString().find_last_of(L"\\/") + 1);
    std::string FileName(WideName.begin(), WideName.end());

    // 마지막 '.'을 찾아 확장자를 제거
    size_t DotPos = FileName.find_last_of('.');
    if (DotPos != std::string::npos)
    {
        OutObjInfo.DisplayName = FileName.substr(0, DotPos);
    }
    else
    {
        OutObjInfo.DisplayName = FileName;
    }

    /**
     * 블렌더 Export 설정
     *   > General
     *       Forward Axis:  Y
     *       Up Axis:       Z
     *   > Geometry
     *       ✅ Triangulated Mesh
     *   > Materials
     *       ✅ PBR Extensions
     *       Path Mode:     Strip
     */

    std::string Line;

    while (std::getline(Obj, Line))
    {
        if (Line.empty() || Line[0] == '#')
        {
            continue;
        }

        std::istringstream LineStream(Line);
        std::string Token;
        LineStream >> Token;

        if (Token == "mtllib")
        {
            LineStream >> Line;
            OutObjInfo.MatName = Line;
            continue;
        }

        if (Token == "usemtl")
        {
            LineStream >> Line;
            FString MatName(Line);

            if (!OutObjInfo.MaterialSubsets.IsEmpty())
            {
                FMaterialSubset& LastSubset = OutObjInfo.MaterialSubsets[OutObjInfo.MaterialSubsets.Num() - 1];
                LastSubset.IndexCount = OutObjInfo.VertexIndices.Num() - LastSubset.IndexStart;
            }

            FMaterialSubset MaterialSubset;
            MaterialSubset.MaterialName = MatName;
            MaterialSubset.IndexStart = OutObjInfo.VertexIndices.Num();
            MaterialSubset.IndexCount = 0;
            OutObjInfo.MaterialSubsets.Add(MaterialSubset);
        }

        if (Token == "g" || Token == "o")
        {
            LineStream >> Line;
            OutObjInfo.GroupName.Add(Line);
            OutObjInfo.NumOfGroup++;
        }

        if (Token == "v") // Vertex
        {
            float X, Y, Z;
            LineStream >> X >> Y >> Z;
            OutObjInfo.Vertices.Add(FVector(X, Y * -1.f, Z));
            continue;
        }

        if (Token == "vn") // Normal
        {
            float NormalX, NormalY, NormalZ;
            LineStream >> NormalX >> NormalY >> NormalZ;
            OutObjInfo.Normals.Add(FVector(NormalX, NormalY * -1.f, NormalZ));
            continue;
        }

        if (Token == "vt") // Texture
        {
            float U, V;
            LineStream >> U >> V;
            OutObjInfo.UVs.Add(FVector2D(U, 1.f - V));
            continue;
        }

        if (Token == "f")
        {
            TArray<uint32> FaceVertexIndices;  // 이번 페이스의 정점 인덱스
            TArray<uint32> FaceNormalIndices;  // 이번 페이스의 법선 인덱스
            TArray<uint32> FaceUVIndices; // 이번 페이스의 텍스처 인덱스

            while (LineStream >> Token)
            {
                std::istringstream TokenStream(Token);
                std::string Part;
                TArray<std::string> FacePieces;

                uint32 VertexIndex = 0;
                uint32 TextureIndex = UINT32_MAX;
                uint32 NormalIndex = UINT32_MAX;

                // v
                if (std::getline(TokenStream, Part, '/'))
                {
                    if (!Part.empty())
                    {
                        VertexIndex = std::stoi(Part) - 1;
                    }
                }

                // vt
                if (std::getline(TokenStream, Part, '/'))
                {
                    if (!Part.empty())
                    {
                        TextureIndex = std::stoi(Part) - 1;
                    }
                }

                // vn
                if (std::getline(TokenStream, Part, '/'))
                {
                    if (!Part.empty())
                    {
                        NormalIndex = std::stoi(Part) - 1;
                    }
                }

                FaceVertexIndices.Add(VertexIndex);
                FaceUVIndices.Add(TextureIndex);
                FaceNormalIndices.Add(NormalIndex);
            }

            if (FaceVertexIndices.Num() == 3) // 삼각형
            {
                // 반시계 방향(오른손 좌표계)을 시계 방향(왼손 좌표계)으로 변환: 0-2-1
                OutObjInfo.VertexIndices.Add(FaceVertexIndices[0]);
                OutObjInfo.VertexIndices.Add(FaceVertexIndices[2]);
                OutObjInfo.VertexIndices.Add(FaceVertexIndices[1]);

                OutObjInfo.UVIndices.Add(FaceUVIndices[0]);
                OutObjInfo.UVIndices.Add(FaceUVIndices[2]);
                OutObjInfo.UVIndices.Add(FaceUVIndices[1]);

                OutObjInfo.NormalIndices.Add(FaceNormalIndices[0]);
                OutObjInfo.NormalIndices.Add(FaceNormalIndices[2]);
                OutObjInfo.NormalIndices.Add(FaceNormalIndices[1]);
            }
            else if (FaceVertexIndices.Num() == 4) // 쿼드
            {
                // 첫 번째 삼각형: 0-2-1
                OutObjInfo.VertexIndices.Add(FaceVertexIndices[0]);
                OutObjInfo.VertexIndices.Add(FaceVertexIndices[2]);
                OutObjInfo.VertexIndices.Add(FaceVertexIndices[1]);

                OutObjInfo.UVIndices.Add(FaceUVIndices[0]);
                OutObjInfo.UVIndices.Add(FaceUVIndices[2]);
                OutObjInfo.UVIndices.Add(FaceUVIndices[1]);

                OutObjInfo.NormalIndices.Add(FaceNormalIndices[0]);
                OutObjInfo.NormalIndices.Add(FaceNormalIndices[2]);
                OutObjInfo.NormalIndices.Add(FaceNormalIndices[1]);

                // 두 번째 삼각형: 0-3-2
                OutObjInfo.VertexIndices.Add(FaceVertexIndices[0]);
                OutObjInfo.VertexIndices.Add(FaceVertexIndices[3]);
                OutObjInfo.VertexIndices.Add(FaceVertexIndices[2]);

                OutObjInfo.UVIndices.Add(FaceUVIndices[0]);
                OutObjInfo.UVIndices.Add(FaceUVIndices[3]);
                OutObjInfo.UVIndices.Add(FaceUVIndices[2]);

                OutObjInfo.NormalIndices.Add(FaceNormalIndices[0]);
                OutObjInfo.NormalIndices.Add(FaceNormalIndices[3]);
                OutObjInfo.NormalIndices.Add(FaceNormalIndices[2]);
            }
        }
    }

    if (!OutObjInfo.MaterialSubsets.IsEmpty())
    {
        FMaterialSubset& LastSubset = OutObjInfo.MaterialSubsets[OutObjInfo.MaterialSubsets.Num() - 1];
        LastSubset.IndexCount = OutObjInfo.VertexIndices.Num() - LastSubset.IndexStart;
    }

    return true;
}

bool FObjLoader::ParseMaterial(FObjInfo& OutObjInfo, FStaticMeshRenderData& OutStaticMeshRenderData)
{
    // Subset
    OutStaticMeshRenderData.MaterialSubsets = OutObjInfo.MaterialSubsets;

    std::ifstream MtlFile(OutObjInfo.FilePath + OutObjInfo.MatName.ToWideString());
    if (!MtlFile.is_open())
    {
        return false;
    }

    std::string Line;
    int32 MaterialIndex = -1;

    while (std::getline(MtlFile, Line))
    {
        if (Line.empty() || Line[0] == '#')
        {
            continue;
        }

        std::istringstream LineStream(Line);
        std::string Token;
        LineStream >> Token;

        // Create new material if token is 'newmtl'
        if (Token == "newmtl")
        {
            LineStream >> Line;
            MaterialIndex++;

            FMaterialInfo Material;
            Material.MaterialName = Line;
            
            constexpr uint32 TexturesNum = static_cast<uint32>(EMaterialTextureSlots::MTS_MAX);
            Material.TextureInfos.SetNum(TexturesNum);

            OutStaticMeshRenderData.Materials.Add(Material);
        }

        if (Token == "Kd")
        {
            float X, Y, Z;
            LineStream >> X >> Y >> Z;
            OutStaticMeshRenderData.Materials[MaterialIndex].DiffuseColor = FVector(X, Y, Z);
        }
        if (Token == "Ks")
        {
            float X, Y, Z;
            LineStream >> X >> Y >> Z;
            OutStaticMeshRenderData.Materials[MaterialIndex].SpecularColor = FVector(X, Y, Z);
        }
        if (Token == "Ka")
        {
            float X, Y, Z;
            LineStream >> X >> Y >> Z;
            OutStaticMeshRenderData.Materials[MaterialIndex].AmbientColor = FVector(X, Y, Z);
        }
        if (Token == "Ke")
        {
            float X, Y, Z;
            LineStream >> X >> Y >> Z;
            OutStaticMeshRenderData.Materials[MaterialIndex].EmissiveColor = FVector(X, Y, Z);
        }
        if (Token == "Ns")
        {
            float X;
            LineStream >> X;
            OutStaticMeshRenderData.Materials[MaterialIndex].Shininess = X;
        }
        if (Token == "Ni")
        {
            float X;
            LineStream >> X;
            OutStaticMeshRenderData.Materials[MaterialIndex].IOR = X;
        }
        if (Token == "d" || Token == "Tr")
        {
            float X;
            LineStream >> X;
            OutStaticMeshRenderData.Materials[MaterialIndex].Transparency = (Token == "Tr") ? X : 1.f - X;
            OutStaticMeshRenderData.Materials[MaterialIndex].bTransparent = true;
        }
        if (Token == "illum")
        {
            uint32 X;
            LineStream >> X;
            OutStaticMeshRenderData.Materials[MaterialIndex].IlluminanceModel = X;
        }
        if (Token == "Pm")
        {
            float X;
            LineStream >> X;
            OutStaticMeshRenderData.Materials[MaterialIndex].Metallic = X;
        }
        if (Token == "Pr")
        {
            float X;
            LineStream >> X;
            OutStaticMeshRenderData.Materials[MaterialIndex].Roughness = X;
        }

        if (Token == "map_Kd")
        {
            constexpr uint32 SlotIdx = static_cast<uint32>(EMaterialTextureSlots::MTS_Diffuse);
            
            LineStream >> Line;
            OutStaticMeshRenderData.Materials[MaterialIndex].TextureInfos[SlotIdx].TextureName = Line;

            FWString TexturePath = OutObjInfo.FilePath + OutStaticMeshRenderData.Materials[MaterialIndex].TextureInfos[SlotIdx].TextureName.ToWideString();
            if (CreateTextureFromFile(TexturePath))
            {
                OutStaticMeshRenderData.Materials[MaterialIndex].TextureInfos[SlotIdx].TexturePath = TexturePath;
                OutStaticMeshRenderData.Materials[MaterialIndex].TextureInfos[SlotIdx].bIsSRGB = true;
                OutStaticMeshRenderData.Materials[MaterialIndex].TextureFlag |= static_cast<uint16>(EMaterialTextureFlags::MTF_Diffuse);
            }
        }
        if (Token == "map_Bump")
        {
            constexpr uint32 SlotIdx = static_cast<uint32>(EMaterialTextureSlots::MTS_Normal);
            
            std::string Option;
            while (LineStream >> Option)
            {
                if (Option == "-bm")
                {
                    float BumpMultiplier;
                    LineStream >> BumpMultiplier;
                    OutStaticMeshRenderData.Materials[MaterialIndex].BumpMultiplier = BumpMultiplier;
                }
                else
                {
                    OutStaticMeshRenderData.Materials[MaterialIndex].TextureInfos[SlotIdx].TextureName = Option;

                    FWString TexturePath = OutObjInfo.FilePath + OutStaticMeshRenderData.Materials[MaterialIndex].TextureInfos[SlotIdx].TextureName.ToWideString();
                    if (CreateTextureFromFile(TexturePath, false))
                    {
                        OutStaticMeshRenderData.Materials[MaterialIndex].TextureInfos[SlotIdx].TexturePath = TexturePath;
                        OutStaticMeshRenderData.Materials[MaterialIndex].TextureInfos[SlotIdx].bIsSRGB = false;
                        OutStaticMeshRenderData.Materials[MaterialIndex].TextureFlag |= static_cast<uint16>(EMaterialTextureFlags::MTF_Normal);
                    }
                }
            }
        }
        if (Token == "map_Ks")
        {
            constexpr uint32 SlotIdx = static_cast<uint32>(EMaterialTextureSlots::MTS_Specular);
            
            LineStream >> Line;
            OutStaticMeshRenderData.Materials[MaterialIndex].TextureInfos[SlotIdx].TextureName = Line;

            FWString TexturePath = OutObjInfo.FilePath + OutStaticMeshRenderData.Materials[MaterialIndex].TextureInfos[SlotIdx].TextureName.ToWideString();
            if (CreateTextureFromFile(TexturePath))
            {
                OutStaticMeshRenderData.Materials[MaterialIndex].TextureInfos[SlotIdx].TexturePath = TexturePath;
                OutStaticMeshRenderData.Materials[MaterialIndex].TextureInfos[SlotIdx].bIsSRGB = true;
                OutStaticMeshRenderData.Materials[MaterialIndex].TextureFlag |= static_cast<uint16>(EMaterialTextureFlags::MTF_Specular);
            }
        }
        if (Token == "map_Ns")
        {
            constexpr uint32 SlotIdx = static_cast<uint32>(EMaterialTextureSlots::MTS_Shininess);
            
            LineStream >> Line;
            OutStaticMeshRenderData.Materials[MaterialIndex].TextureInfos[SlotIdx].TextureName = Line;

            FWString TexturePath = OutObjInfo.FilePath + OutStaticMeshRenderData.Materials[MaterialIndex].TextureInfos[SlotIdx].TextureName.ToWideString();
            if (CreateTextureFromFile(TexturePath, false))
            {
                OutStaticMeshRenderData.Materials[MaterialIndex].TextureInfos[SlotIdx].TexturePath = TexturePath;
                OutStaticMeshRenderData.Materials[MaterialIndex].TextureInfos[SlotIdx].bIsSRGB = false;
                OutStaticMeshRenderData.Materials[MaterialIndex].TextureFlag |= static_cast<uint16>(EMaterialTextureFlags::MTF_Shininess);
            }
        }
        if (Token == "map_Ka")
        {
            constexpr uint32 SlotIdx = static_cast<uint32>(EMaterialTextureSlots::MTS_Ambient);
            
            LineStream >> Line;
            OutStaticMeshRenderData.Materials[MaterialIndex].TextureInfos[SlotIdx].TextureName = Line;

            FWString TexturePath = OutObjInfo.FilePath + OutStaticMeshRenderData.Materials[MaterialIndex].TextureInfos[SlotIdx].TextureName.ToWideString();
            if (CreateTextureFromFile(TexturePath))
            {
                OutStaticMeshRenderData.Materials[MaterialIndex].TextureInfos[SlotIdx].TexturePath = TexturePath;
                OutStaticMeshRenderData.Materials[MaterialIndex].TextureInfos[SlotIdx].bIsSRGB = true;
                OutStaticMeshRenderData.Materials[MaterialIndex].TextureFlag |= static_cast<uint16>(EMaterialTextureFlags::MTF_Ambient);
            }
        }
        if (Token == "map_Ke")
        {
            constexpr uint32 SlotIdx = static_cast<uint32>(EMaterialTextureSlots::MTS_Emissive);
            
            LineStream >> Line;
            OutStaticMeshRenderData.Materials[MaterialIndex].TextureInfos[SlotIdx].TextureName = Line;

            FWString TexturePath = OutObjInfo.FilePath + OutStaticMeshRenderData.Materials[MaterialIndex].TextureInfos[SlotIdx].TextureName.ToWideString();
            if (CreateTextureFromFile(TexturePath))
            {
                OutStaticMeshRenderData.Materials[MaterialIndex].TextureInfos[SlotIdx].TexturePath = TexturePath;
                OutStaticMeshRenderData.Materials[MaterialIndex].TextureInfos[SlotIdx].bIsSRGB = true;
                OutStaticMeshRenderData.Materials[MaterialIndex].TextureFlag |= static_cast<uint16>(EMaterialTextureFlags::MTF_Emissive);
            }
        }
        if (Token == "map_Pm")
        {
            constexpr uint32 SlotIdx = static_cast<uint32>(EMaterialTextureSlots::MTS_Metallic);
            
            LineStream >> Line;
            OutStaticMeshRenderData.Materials[MaterialIndex].TextureInfos[SlotIdx].TextureName = Line;

            FWString TexturePath = OutObjInfo.FilePath + OutStaticMeshRenderData.Materials[MaterialIndex].TextureInfos[SlotIdx].TextureName.ToWideString();
            if (CreateTextureFromFile(TexturePath, false))
            {
                OutStaticMeshRenderData.Materials[MaterialIndex].TextureInfos[SlotIdx].TexturePath = TexturePath;
                OutStaticMeshRenderData.Materials[MaterialIndex].TextureInfos[SlotIdx].bIsSRGB = false;
                OutStaticMeshRenderData.Materials[MaterialIndex].TextureFlag |= static_cast<uint16>(EMaterialTextureFlags::MTF_Metallic);
            }
        }
        if (Token == "map_Pr")
        {
            constexpr uint32 SlotIdx = static_cast<uint32>(EMaterialTextureSlots::MTS_Roughness);
            
            LineStream >> Line;
            OutStaticMeshRenderData.Materials[MaterialIndex].TextureInfos[SlotIdx].TextureName = Line;

            FWString TexturePath = OutObjInfo.FilePath + OutStaticMeshRenderData.Materials[MaterialIndex].TextureInfos[SlotIdx].TextureName.ToWideString();
            if (CreateTextureFromFile(TexturePath, false))
            {
                OutStaticMeshRenderData.Materials[MaterialIndex].TextureInfos[SlotIdx].TexturePath = TexturePath;
                OutStaticMeshRenderData.Materials[MaterialIndex].TextureInfos[SlotIdx].bIsSRGB = false;
                OutStaticMeshRenderData.Materials[MaterialIndex].TextureFlag |= static_cast<uint16>(EMaterialTextureFlags::MTF_Roughness);
            }
        }
        // TODO: map_d 또는 map_Tr은 나중에 필요할때 구현
    }

    return true;
}

bool FObjLoader::ConvertToStaticMesh(const FObjInfo& RawData, FStaticMeshRenderData& OutStaticMesh)
{
    OutStaticMesh.ObjectName = RawData.ObjectName;
    OutStaticMesh.DisplayName = RawData.DisplayName;

    // 고유 정점을 기반으로 FVertexSimple 배열 생성
    TMap<std::string, uint32> IndexMap; // 중복 체크용

    for (int32 Idx = 0; Idx < RawData.VertexIndices.Num(); Idx++)
    {
        const uint32 VertexIndex = RawData.VertexIndices[Idx];
        const uint32 UVIndex = RawData.UVIndices[Idx];
        const uint32 NormalIndex = RawData.NormalIndices[Idx];

        uint32 MaterialIndex = 0;
        for (int32 SubsetIdx = 0; SubsetIdx < OutStaticMesh.MaterialSubsets.Num(); SubsetIdx++)
        {
            const FMaterialSubset& Subset = OutStaticMesh.MaterialSubsets[SubsetIdx];
            if (Subset.IndexStart <= Idx && Idx < Subset.IndexStart + Subset.IndexCount)
            {
                MaterialIndex = Subset.MaterialIndex;
                break;
            }
        }

        // 키 생성 (v/vt/vn 조합)
        std::string Key = std::to_string(VertexIndex) + "/" + std::to_string(UVIndex) + "/" + std::to_string(NormalIndex);

        uint32 FinalIndex;
        if (IndexMap.Contains(Key))
        {
            FinalIndex = IndexMap[Key];
        }
        else
        {
            FStaticMeshVertex StaticMeshVertex = {};
            StaticMeshVertex.MaterialIndex = MaterialIndex;
            StaticMeshVertex.X = RawData.Vertices[VertexIndex].X;
            StaticMeshVertex.Y = RawData.Vertices[VertexIndex].Y;
            StaticMeshVertex.Z = RawData.Vertices[VertexIndex].Z;

            StaticMeshVertex.R = 0.7f; StaticMeshVertex.G = 0.7f; StaticMeshVertex.B = 0.7f; StaticMeshVertex.A = 1.0f; // 기본 색상

            if (UVIndex != UINT32_MAX && UVIndex < RawData.UVs.Num())
            {
                StaticMeshVertex.U = RawData.UVs[UVIndex].X;
                StaticMeshVertex.V = RawData.UVs[UVIndex].Y;
            }

            if (NormalIndex != UINT32_MAX && NormalIndex < RawData.Normals.Num())
            {
                StaticMeshVertex.NormalX = RawData.Normals[NormalIndex].X;
                StaticMeshVertex.NormalY = RawData.Normals[NormalIndex].Y;
                StaticMeshVertex.NormalZ = RawData.Normals[NormalIndex].Z;
            }

            FinalIndex = OutStaticMesh.Vertices.Num();
            IndexMap[Key] = FinalIndex;
            OutStaticMesh.Vertices.Add(StaticMeshVertex);
        }

        OutStaticMesh.Indices.Add(FinalIndex);
    }

    // Tangent
    for (int32 Idx = 0; Idx < OutStaticMesh.Indices.Num(); Idx += 3)
    {
        FStaticMeshVertex& Vertex0 = OutStaticMesh.Vertices[OutStaticMesh.Indices[Idx]];
        FStaticMeshVertex& Vertex1 = OutStaticMesh.Vertices[OutStaticMesh.Indices[Idx + 1]];
        FStaticMeshVertex& Vertex2 = OutStaticMesh.Vertices[OutStaticMesh.Indices[Idx + 2]];

        CalculateTangent(Vertex0, Vertex1, Vertex2);
        CalculateTangent(Vertex1, Vertex2, Vertex0);
        CalculateTangent(Vertex2, Vertex0, Vertex1);
    }

    // Calculate StaticMesh BoundingBox
    ComputeBoundingBox(OutStaticMesh.Vertices, OutStaticMesh.BoundingBoxMin, OutStaticMesh.BoundingBoxMax);

    return true;
}

bool FObjLoader::CreateTextureFromFile(const FWString& Filename, bool bIsSRGB)
{
    if (FEngineLoop::ResourceManager.GetTexture(Filename))
    {
        return true;
    }

    HRESULT hr = FEngineLoop::ResourceManager.LoadTextureFromFile(FEngineLoop::GraphicDevice.Device, Filename.c_str(), bIsSRGB);

    if (FAILED(hr))
    {
        return false;
    }

    return true;
}

void FObjLoader::ComputeBoundingBox(const TArray<FStaticMeshVertex>& InVertices, FVector& OutMinVector, FVector& OutMaxVector)
{
    FVector MinVector = { FLT_MAX, FLT_MAX, FLT_MAX };
    FVector MaxVector = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

    for (int32 Idx = 0; Idx < InVertices.Num(); Idx++)
    {
        MinVector.X = std::min(MinVector.X, InVertices[Idx].X);
        MinVector.Y = std::min(MinVector.Y, InVertices[Idx].Y);
        MinVector.Z = std::min(MinVector.Z, InVertices[Idx].Z);

        MaxVector.X = std::max(MaxVector.X, InVertices[Idx].X);
        MaxVector.Y = std::max(MaxVector.Y, InVertices[Idx].Y);
        MaxVector.Z = std::max(MaxVector.Z, InVertices[Idx].Z);
    }

    if (MinVector == FVector{ FLT_MAX, FLT_MAX, FLT_MAX } && MaxVector == FVector{ -FLT_MAX, -FLT_MAX, -FLT_MAX })
    {
        OutMinVector = FVector::ZeroVector;
        OutMaxVector = FVector::ZeroVector;
    }
    else
    {
        OutMinVector = MinVector;
        OutMaxVector = MaxVector;
    }
}

void FObjLoader::CalculateTangent(FStaticMeshVertex& PivotVertex, const FStaticMeshVertex& Vertex1, const FStaticMeshVertex& Vertex2)
{
    const float s1 = Vertex1.U - PivotVertex.U;
    const float t1 = Vertex1.V - PivotVertex.V;
    const float s2 = Vertex2.U - PivotVertex.U;
    const float t2 = Vertex2.V - PivotVertex.V;
    const float E1x = Vertex1.X - PivotVertex.X;
    const float E1y = Vertex1.Y - PivotVertex.Y;
    const float E1z = Vertex1.Z - PivotVertex.Z;
    const float E2x = Vertex2.X - PivotVertex.X;
    const float E2y = Vertex2.Y - PivotVertex.Y;
    const float E2z = Vertex2.Z - PivotVertex.Z;

    const float Denominator = s1 * t2 - s2 * t1;
    FVector Tangent(1, 0, 0);
    FVector BiTangent(0, 1, 0);
    FVector Normal(PivotVertex.NormalX, PivotVertex.NormalY, PivotVertex.NormalZ);
    
    if (FMath::Abs(Denominator) > SMALL_NUMBER)
    {
        // 정상적인 계산 진행
        const float f = 1.f / Denominator;
        
        const float Tx = f * (t2 * E1x - t1 * E2x);
        const float Ty = f * (t2 * E1y - t1 * E2y);
        const float Tz = f * (t2 * E1z - t1 * E2z);
        Tangent = FVector(Tx, Ty, Tz).GetSafeNormal();

        const float Bx = f * (-s2 * E1x + s1 * E2x);
        const float By = f * (-s2 * E1y + s1 * E2y);
        const float Bz = f * (-s2 * E1z + s1 * E2z);
        BiTangent = FVector(Bx, By, Bz).GetSafeNormal();
    }
    else
    {
        // 대체 탄젠트 계산 방법
        // 방법 1: 다른 방향에서 탄젠트 계산 시도
        FVector Edge1(E1x, E1y, E1z);
        FVector Edge2(E2x, E2y, E2z);
    
        // 기하학적 접근: 두 에지 사이의 각도 이등분선 사용
        Tangent = (Edge1.GetSafeNormal() + Edge2.GetSafeNormal()).GetSafeNormal();
    
        // 만약 두 에지가 평행하거나 반대 방향이면 다른 방법 사용
        if (Tangent.IsNearlyZero())
        {
            // TODO: 기본 축 방향 중 하나 선택 (메시의 주 방향에 따라 선택)
            Tangent = FVector(1.0f, 0.0f, 0.0f);
        }
    }

    Tangent = (Tangent - Normal * FVector::DotProduct(Normal, Tangent)).GetSafeNormal();
    
    const float Sign = (FVector::DotProduct(FVector::CrossProduct(Normal, Tangent), BiTangent) < 0.f) ? -1.f : 1.f;

    PivotVertex.TangentX = Tangent.X;
    PivotVertex.TangentY = Tangent.Y;
    PivotVertex.TangentZ = Tangent.Z;
    PivotVertex.TangentW = Sign;
}

FStaticMeshRenderData* FObjManager::LoadObjStaticMeshAsset(const FString& PathFileName)
{
    if ( const auto It = ObjStaticMeshMap.Find(PathFileName))
    {
        return *It;
    }
    

    FWString BinaryPath = (PathFileName + ".bin").ToWideString();
    if (std::ifstream(BinaryPath).good())
    {
        FStaticMeshRenderData* NewStaticMesh = new FStaticMeshRenderData();
        if (LoadStaticMeshFromBinary(BinaryPath, *NewStaticMesh))
        {
            ObjStaticMeshMap.Add(PathFileName, NewStaticMesh);
            return NewStaticMesh;
        }
        else
        {
            delete NewStaticMesh;
        }
    }

    // Parse OBJ
    FObjInfo NewObjInfo;
    bool Result = FObjLoader::ParseObj(PathFileName, NewObjInfo);

    if (!Result)
    {
        return nullptr;
    }

    // Material
    FStaticMeshRenderData* NewStaticMesh = new FStaticMeshRenderData();
    if (NewObjInfo.MaterialSubsets.Num() > 0)
    {
        Result = FObjLoader::ParseMaterial(NewObjInfo, *NewStaticMesh);

        if (!Result)
        {
            delete NewStaticMesh;
            return nullptr;
        }

        CombineMaterialIndex(*NewStaticMesh);

        for (int MaterialIndex = 0; MaterialIndex < NewStaticMesh->Materials.Num(); MaterialIndex++) {
            CreateMaterial(NewStaticMesh->Materials[MaterialIndex]);
        }
    }

    // Convert FStaticMeshRenderData
    Result = FObjLoader::ConvertToStaticMesh(NewObjInfo, *NewStaticMesh);
    if (!Result)
    {
        delete NewStaticMesh;
        return nullptr;
    }

    SaveStaticMeshToBinary(BinaryPath, *NewStaticMesh); 
    ObjStaticMeshMap.Add(PathFileName, NewStaticMesh);
    return NewStaticMesh;
}

void FObjManager::CombineMaterialIndex(FStaticMeshRenderData& OutFStaticMesh)
{
    for (int32 Idx = 0; Idx < OutFStaticMesh.MaterialSubsets.Num(); Idx++)
    {
        FString MatName = OutFStaticMesh.MaterialSubsets[Idx].MaterialName;
        for (int32 j = 0; j < OutFStaticMesh.Materials.Num(); j++)
        {
            if (OutFStaticMesh.Materials[j].MaterialName == MatName)
            {
                OutFStaticMesh.MaterialSubsets[Idx].MaterialIndex = j;
                break;
            }
        }
    }
}

bool FObjManager::SaveStaticMeshToBinary(const FWString& FilePath, const FStaticMeshRenderData& StaticMesh)
{
    if (FilePath == FString("Contents/ParticleMaterials/DummyObj.obj.bin").ToWideString())
    {
        // 머티리얼용 더미 obj 파일은 bin 저장 안함.
        return false;
    }
    
    std::ofstream File(FilePath, std::ios::binary);
    if (!File.is_open())
    {
        assert("CAN'T SAVE STATIC MESH BINARY FILE");
        return false;
    }

    // Object Name
    Serializer::WriteFWString(File, StaticMesh.ObjectName);

    // Display Name
    Serializer::WriteFString(File, StaticMesh.DisplayName);

    // Vertices
    uint32 VertexCount = StaticMesh.Vertices.Num();
    File.write(reinterpret_cast<const char*>(&VertexCount), sizeof(VertexCount));
    File.write(reinterpret_cast<const char*>(StaticMesh.Vertices.GetData()), VertexCount * sizeof(FStaticMeshVertex));

    // Indices
    uint32 IndexCount = StaticMesh.Indices.Num();
    File.write(reinterpret_cast<const char*>(&IndexCount), sizeof(IndexCount));
    File.write(reinterpret_cast<const char*>(StaticMesh.Indices.GetData()), IndexCount * sizeof(UINT));

    // Materials
    uint32 MaterialCount = StaticMesh.Materials.Num();
    File.write(reinterpret_cast<const char*>(&MaterialCount), sizeof(MaterialCount));
    for (const FMaterialInfo& Material : StaticMesh.Materials)
    {
        Serializer::WriteFString(File, Material.MaterialName);
        
        File.write(reinterpret_cast<const char*>(&Material.TextureFlag), sizeof(Material.TextureFlag));
        
        File.write(reinterpret_cast<const char*>(&Material.bTransparent), sizeof(Material.bTransparent));
        File.write(reinterpret_cast<const char*>(&Material.DiffuseColor), sizeof(Material.DiffuseColor));
        File.write(reinterpret_cast<const char*>(&Material.SpecularColor), sizeof(Material.SpecularColor));
        File.write(reinterpret_cast<const char*>(&Material.AmbientColor), sizeof(Material.AmbientColor));
        File.write(reinterpret_cast<const char*>(&Material.EmissiveColor), sizeof(Material.EmissiveColor));
        
        File.write(reinterpret_cast<const char*>(&Material.Shininess), sizeof(Material.Shininess));
        File.write(reinterpret_cast<const char*>(&Material.IOR), sizeof(Material.IOR));
        File.write(reinterpret_cast<const char*>(&Material.Transparency), sizeof(Material.Transparency));
        File.write(reinterpret_cast<const char*>(&Material.BumpMultiplier), sizeof(Material.BumpMultiplier));
        File.write(reinterpret_cast<const char*>(&Material.IlluminanceModel), sizeof(Material.IlluminanceModel));

        File.write(reinterpret_cast<const char*>(&Material.Metallic), sizeof(Material.Metallic));
        File.write(reinterpret_cast<const char*>(&Material.Roughness), sizeof(Material.Roughness));

        for (uint8 i = 0; i < static_cast<uint8>(EMaterialTextureSlots::MTS_MAX); ++i)
        {
            Serializer::WriteFString(File, Material.TextureInfos[i].TextureName);
            Serializer::WriteFWString(File, Material.TextureInfos[i].TexturePath);
            File.write(reinterpret_cast<const char*>(&Material.TextureInfos[i].bIsSRGB), sizeof(Material.TextureInfos[i].bIsSRGB));
        }
    }

    // Material Subsets
    uint32 SubsetCount = StaticMesh.MaterialSubsets.Num();
    File.write(reinterpret_cast<const char*>(&SubsetCount), sizeof(SubsetCount));
    for (const FMaterialSubset& Subset : StaticMesh.MaterialSubsets)
    {
        Serializer::WriteFString(File, Subset.MaterialName);
        File.write(reinterpret_cast<const char*>(&Subset.IndexStart), sizeof(Subset.IndexStart));
        File.write(reinterpret_cast<const char*>(&Subset.IndexCount), sizeof(Subset.IndexCount));
        File.write(reinterpret_cast<const char*>(&Subset.MaterialIndex), sizeof(Subset.MaterialIndex));
    }

    // Bounding Box
    File.write(reinterpret_cast<const char*>(&StaticMesh.BoundingBoxMin), sizeof(FVector));
    File.write(reinterpret_cast<const char*>(&StaticMesh.BoundingBoxMax), sizeof(FVector));

    File.close();
    return true;
}

bool FObjManager::LoadStaticMeshFromBinary(const FWString& FilePath, FStaticMeshRenderData& OutStaticMesh)
{
    std::ifstream File(FilePath, std::ios::binary);
    if (!File.is_open())
    {
        assert("CAN'T OPEN STATIC MESH BINARY FILE");
        return false;
    }

    TArray<TPair<FWString, bool>> Textures;

    // Object Name
    Serializer::ReadFWString(File, OutStaticMesh.ObjectName);

    //// Path Name
    //Serializer::ReadFWString(File, OutStaticMesh.PathName);

    // Display Name
    Serializer::ReadFString(File, OutStaticMesh.DisplayName);

    // Vertices
    uint32 VertexCount = 0;
    File.read(reinterpret_cast<char*>(&VertexCount), sizeof(VertexCount));
    OutStaticMesh.Vertices.SetNum(VertexCount);
    File.read(reinterpret_cast<char*>(OutStaticMesh.Vertices.GetData()), VertexCount * sizeof(FStaticMeshVertex));

    // Indices
    uint32 IndexCount = 0;
    File.read(reinterpret_cast<char*>(&IndexCount), sizeof(IndexCount));
    OutStaticMesh.Indices.SetNum(IndexCount);
    File.read(reinterpret_cast<char*>(OutStaticMesh.Indices.GetData()), IndexCount * sizeof(UINT));

    // Material
    uint32 MaterialCount = 0;
    File.read(reinterpret_cast<char*>(&MaterialCount), sizeof(MaterialCount));
    OutStaticMesh.Materials.SetNum(MaterialCount);
    for (FMaterialInfo& Material : OutStaticMesh.Materials)
    {
        Serializer::ReadFString(File, Material.MaterialName);
        File.read(reinterpret_cast<char*>(&Material.TextureFlag), sizeof(Material.TextureFlag));
        
        File.read(reinterpret_cast<char*>(&Material.bTransparent), sizeof(Material.bTransparent));
        File.read(reinterpret_cast<char*>(&Material.DiffuseColor), sizeof(Material.DiffuseColor));
        File.read(reinterpret_cast<char*>(&Material.SpecularColor), sizeof(Material.SpecularColor));
        File.read(reinterpret_cast<char*>(&Material.AmbientColor), sizeof(Material.AmbientColor));
        File.read(reinterpret_cast<char*>(&Material.EmissiveColor), sizeof(Material.EmissiveColor));
        
        File.read(reinterpret_cast<char*>(&Material.Shininess), sizeof(Material.Shininess));
        File.read(reinterpret_cast<char*>(&Material.IOR), sizeof(Material.IOR));
        File.read(reinterpret_cast<char*>(&Material.Transparency), sizeof(Material.Transparency));
        File.read(reinterpret_cast<char*>(&Material.BumpMultiplier), sizeof(Material.BumpMultiplier));
        File.read(reinterpret_cast<char*>(&Material.IlluminanceModel), sizeof(Material.IlluminanceModel));

        File.read(reinterpret_cast<char*>(&Material.Metallic), sizeof(Material.Metallic));
        File.read(reinterpret_cast<char*>(&Material.Roughness), sizeof(Material.Roughness));

        uint8 TextureNum = static_cast<uint8>(EMaterialTextureSlots::MTS_MAX);
        Material.TextureInfos.SetNum(TextureNum);
        for (uint8 i = 0; i < TextureNum; ++i)
        {
            Serializer::ReadFString(File, Material.TextureInfos[i].TextureName);
            Serializer::ReadFWString(File, Material.TextureInfos[i].TexturePath);
            File.read(reinterpret_cast<char*>(&Material.TextureInfos[i].bIsSRGB), sizeof(Material.TextureInfos[i].bIsSRGB));

            Textures.AddUnique({Material.TextureInfos[i].TexturePath, Material.TextureInfos[i].bIsSRGB});
        }
    }

    // Material Subset
    uint32 SubsetCount = 0;
    File.read(reinterpret_cast<char*>(&SubsetCount), sizeof(SubsetCount));
    OutStaticMesh.MaterialSubsets.SetNum(SubsetCount);
    for (FMaterialSubset& Subset : OutStaticMesh.MaterialSubsets)
    {
        Serializer::ReadFString(File, Subset.MaterialName);
        File.read(reinterpret_cast<char*>(&Subset.IndexStart), sizeof(Subset.IndexStart));
        File.read(reinterpret_cast<char*>(&Subset.IndexCount), sizeof(Subset.IndexCount));
        File.read(reinterpret_cast<char*>(&Subset.MaterialIndex), sizeof(Subset.MaterialIndex));
    }

    // Bounding Box
    File.read(reinterpret_cast<char*>(&OutStaticMesh.BoundingBoxMin), sizeof(FVector));
    File.read(reinterpret_cast<char*>(&OutStaticMesh.BoundingBoxMax), sizeof(FVector));

    File.close();

    // Texture Load
    if (Textures.Num() > 0)
    {
        for (const TPair<FWString, bool>& Texture : Textures)
        {
            if (FEngineLoop::ResourceManager.GetTexture(Texture.Key) == nullptr)
            {
                FEngineLoop::ResourceManager.LoadTextureFromFile(FEngineLoop::GraphicDevice.Device, Texture.Key.c_str(), Texture.Value);
            }
        }
    }

    return true;
}

UMaterial* FObjManager::CreateMaterial(const FMaterialInfo& MaterialInfo)
{
    if (MaterialMap[MaterialInfo.MaterialName] != nullptr)
    {
        return MaterialMap[MaterialInfo.MaterialName];
    }

    UMaterial* NewMaterial = FObjectFactory::ConstructObject<UMaterial>(nullptr); // Material은 Outer가 없이 따로 관리되는 객체이므로 Outer가 없음으로 설정. 추후 Garbage Collection이 추가되면 AssetManager를 생성해서 관리.
    NewMaterial->SetMaterialInfo(MaterialInfo);
    MaterialMap.Add(MaterialInfo.MaterialName, NewMaterial);
    return NewMaterial;
}

UMaterial* FObjManager::GetMaterial(const FString& Name)
{
    if (MaterialMap.Contains(Name))
    {
        return MaterialMap[Name];   
    }
    return nullptr;
}

UStaticMesh* FObjManager::CreateStaticMesh(const FString& FilePath)
{
    FStaticMeshRenderData* StaticMeshRenderData = FObjManager::LoadObjStaticMeshAsset(FilePath);

    if (StaticMeshRenderData == nullptr)
    {
        return nullptr;
    }

    UStaticMesh* StaticMesh = GetStaticMesh(StaticMeshRenderData->ObjectName);
    if (StaticMesh != nullptr)
    {
        return StaticMesh;
    }

    StaticMesh = FObjectFactory::ConstructObject<UStaticMesh>(nullptr); // TODO: 추후 AssetManager를 생성해서 관리.
    StaticMesh->SetData(StaticMeshRenderData);

    StaticMeshMap.Add(StaticMeshRenderData->ObjectName, StaticMesh); // TODO: 장기적으로 보면 파일 이름 대신 경로를 Key로 사용하는게 좋음.
    return StaticMesh;
}

UStaticMesh* FObjManager::GetStaticMesh(const FWString& Name)
{
    return StaticMeshMap[Name];
}
